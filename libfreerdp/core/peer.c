/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * RDP Server Peer
 *
 * Copyright 2011 Vic Lee
 * Copyright 2014 DI (FH) Martin Haimberger <martin.haimberger@thincast.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <winpr/crt.h>
#include <winpr/winsock.h>

#include "info.h"
#include "certificate.h"

#include <freerdp/log.h>

#include "rdp.h"
#include "peer.h"

#define TAG FREERDP_TAG("core.peer")

static HANDLE freerdp_peer_virtual_channel_open(freerdp_peer* client, const char* name,
                                                UINT32 flags)
{
	int length;
	UINT32 index;
	BOOL joined = FALSE;
	rdpMcsChannel* mcsChannel = NULL;
	rdpPeerChannel* peerChannel = NULL;
	rdpMcs* mcs = client->context->rdp->mcs;

	if (flags & WTS_CHANNEL_OPTION_DYNAMIC)
		return NULL; /* not yet supported */

	length = strnlen(name, 9);

	if (length > 8)
		return NULL; /* SVC maximum name length is 8 */

	for (index = 0; index < mcs->channelCount; index++)
	{
		mcsChannel = &(mcs->channels[index]);

		if (!mcsChannel->joined)
			continue;

		if (_strnicmp(name, mcsChannel->Name, length) == 0)
		{
			joined = TRUE;
			break;
		}
	}

	if (!joined)
		return NULL; /* channel is not joined */

	peerChannel = (rdpPeerChannel*)mcsChannel->handle;

	if (peerChannel)
	{
		/* channel is already open */
		return (HANDLE)peerChannel;
	}

	peerChannel = (rdpPeerChannel*)calloc(1, sizeof(rdpPeerChannel));

	if (peerChannel)
	{
		peerChannel->index = index;
		peerChannel->client = client;
		peerChannel->channelFlags = flags;
		peerChannel->channelId = mcsChannel->ChannelId;
		peerChannel->mcsChannel = mcsChannel;
		mcsChannel->handle = (void*)peerChannel;
	}

	return (HANDLE)peerChannel;
}

static BOOL freerdp_peer_virtual_channel_close(freerdp_peer* client, HANDLE hChannel)
{
	rdpMcsChannel* mcsChannel = NULL;
	rdpPeerChannel* peerChannel = NULL;

	if (!hChannel)
		return FALSE;

	peerChannel = (rdpPeerChannel*)hChannel;
	mcsChannel = peerChannel->mcsChannel;
	mcsChannel->handle = NULL;
	free(peerChannel);
	return TRUE;
}

static int freerdp_peer_virtual_channel_write(freerdp_peer* client, HANDLE hChannel,
                                              const BYTE* buffer, UINT32 length)
{
	wStream* s;
	UINT32 flags;
	UINT32 chunkSize;
	UINT32 maxChunkSize;
	UINT32 totalLength;
	rdpPeerChannel* peerChannel;
	rdpMcsChannel* mcsChannel;
	rdpRdp* rdp = client->context->rdp;

	if (!hChannel)
		return -1;

	peerChannel = (rdpPeerChannel*)hChannel;
	mcsChannel = peerChannel->mcsChannel;

	if (peerChannel->channelFlags & WTS_CHANNEL_OPTION_DYNAMIC)
		return -1; /* not yet supported */

	maxChunkSize = rdp->settings->VirtualChannelChunkSize;
	totalLength = length;
	flags = CHANNEL_FLAG_FIRST;

	while (length > 0)
	{
		s = rdp_send_stream_init(rdp);

		if (!s)
			return -1;

		if (length > maxChunkSize)
		{
			chunkSize = rdp->settings->VirtualChannelChunkSize;
		}
		else
		{
			chunkSize = length;
			flags |= CHANNEL_FLAG_LAST;
		}

		if (mcsChannel->options & CHANNEL_OPTION_SHOW_PROTOCOL)
			flags |= CHANNEL_FLAG_SHOW_PROTOCOL;

		Stream_Write_UINT32(s, totalLength);
		Stream_Write_UINT32(s, flags);

		if (!Stream_EnsureRemainingCapacity(s, chunkSize))
		{
			Stream_Release(s);
			return -1;
		}

		Stream_Write(s, buffer, chunkSize);

		if (!rdp_send(rdp, s, peerChannel->channelId))
			return -1;

		buffer += chunkSize;
		length -= chunkSize;
		flags = 0;
	}

	return 1;
}

static void* freerdp_peer_virtual_channel_get_data(freerdp_peer* client, HANDLE hChannel)
{
	rdpPeerChannel* peerChannel = (rdpPeerChannel*)hChannel;

	if (!hChannel)
		return NULL;

	return peerChannel->extra;
}

static int freerdp_peer_virtual_channel_set_data(freerdp_peer* client, HANDLE hChannel, void* data)
{
	rdpPeerChannel* peerChannel = (rdpPeerChannel*)hChannel;

	if (!hChannel)
		return -1;

	peerChannel->extra = data;
	return 1;
}

static BOOL freerdp_peer_initialize(freerdp_peer* client)
{
	rdpRdp* rdp = client->context->rdp;
	rdpSettings* settings = rdp->settings;
	settings->ServerMode = TRUE;
	settings->FrameAcknowledge = 0;
	settings->LocalConnection = client->local;

	if (settings->RdpKeyFile)
	{
		settings->RdpServerRsaKey = key_new(settings->RdpKeyFile);

		if (!settings->RdpServerRsaKey)
		{
			WLog_ERR(TAG, "invalid RDP key file %s", settings->RdpKeyFile);
			return FALSE;
		}
	}
	else if (settings->RdpKeyContent)
	{
		settings->RdpServerRsaKey = key_new_from_content(settings->RdpKeyContent, NULL);

		if (!settings->RdpServerRsaKey)
		{
			WLog_ERR(TAG, "invalid RDP key content");
			return FALSE;
		}
	}

	return TRUE;
}

static BOOL freerdp_peer_get_fds(freerdp_peer* client, void** rfds, int* rcount)
{
	rdpTransport* transport = client->context->rdp->transport;
	transport_get_fds(transport, rfds, rcount);
	return TRUE;
}

static HANDLE freerdp_peer_get_event_handle(freerdp_peer* client)
{
	HANDLE hEvent = NULL;
	rdpTransport* transport = client->context->rdp->transport;
	BIO_get_event(transport->frontBio, &hEvent);
	return hEvent;
}

static DWORD freerdp_peer_get_event_handles(freerdp_peer* client, HANDLE* events, DWORD count)
{
	return transport_get_event_handles(client->context->rdp->transport, events, count);
}

static BOOL freerdp_peer_check_fds(freerdp_peer* peer)
{
	int status;
	rdpRdp* rdp;
	rdp = peer->context->rdp;
	status = rdp_check_fds(rdp);

	if (status < 0)
		return FALSE;

	return TRUE;
}

static BOOL peer_recv_data_pdu(freerdp_peer* client, wStream* s, UINT16 totalLength)
{
	BYTE type;
	UINT16 length;
	UINT32 share_id;
	BYTE compressed_type;
	UINT16 compressed_len;

	if (!rdp_read_share_data_header(s, &length, &type, &share_id, &compressed_type,
	                                &compressed_len))
		return FALSE;

#ifdef WITH_DEBUG_RDP
	WLog_DBG(TAG, "recv %s Data PDU (0x%02" PRIX8 "), length: %" PRIu16 "",
	         data_pdu_type_to_string(type), type, length);
#endif

	switch (type)
	{
		case DATA_PDU_TYPE_SYNCHRONIZE:
			if (!rdp_recv_client_synchronize_pdu(client->context->rdp, s))
				return FALSE;

			break;

		case DATA_PDU_TYPE_CONTROL:
			if (!rdp_server_accept_client_control_pdu(client->context->rdp, s))
				return FALSE;

			break;

		case DATA_PDU_TYPE_INPUT:
			if (!input_recv(client->context->rdp->input, s))
				return FALSE;

			break;

		case DATA_PDU_TYPE_BITMAP_CACHE_PERSISTENT_LIST:
			if (!rdp_server_accept_client_persistent_key_list_pdu(client->context->rdp, s))
				return FALSE;
			break;

		case DATA_PDU_TYPE_FONT_LIST:
			if (!rdp_server_accept_client_font_list_pdu(client->context->rdp, s))
				return FALSE;

			break;

		case DATA_PDU_TYPE_SHUTDOWN_REQUEST:
			mcs_send_disconnect_provider_ultimatum(client->context->rdp->mcs);
			return FALSE;

		case DATA_PDU_TYPE_FRAME_ACKNOWLEDGE:
			if (Stream_GetRemainingLength(s) < 4)
				return FALSE;

			Stream_Read_UINT32(s, client->ack_frame_id);
			IFCALL(client->update->SurfaceFrameAcknowledge, client->update->context,
			       client->ack_frame_id);
			break;

		case DATA_PDU_TYPE_REFRESH_RECT:
			if (!update_read_refresh_rect(client->update, s))
				return FALSE;

			break;

		case DATA_PDU_TYPE_SUPPRESS_OUTPUT:
			if (!update_read_suppress_output(client->update, s))
				return FALSE;

			break;

		default:
			//WLog_ERR(TAG, "Data PDU type %" PRIu8 "", type);
			break;
	}

	return TRUE;
}

static int peer_recv_tpkt_pdu(freerdp_peer* client, wStream* s)
{
	rdpRdp* rdp;
	UINT16 length;
	UINT16 pduType;
	UINT16 pduSource;
	UINT16 channelId;
	UINT16 securityFlags = 0;
	rdp = client->context->rdp;

	if (!rdp_read_header(rdp, s, &length, &channelId))
	{
		WLog_ERR(TAG, "Incorrect RDP header.");
		return -1;
	}

	rdp->inPackets++;
	if (freerdp_shall_disconnect_context(rdp->context))
		return 0;

	if (rdp->settings->UseRdpSecurityLayer)
	{
		if (!rdp_read_security_header(s, &securityFlags, &length))
			return -1;

		if (securityFlags & SEC_ENCRYPT)
		{
			if (!rdp_decrypt(rdp, s, &length, securityFlags))
			{
				WLog_ERR(TAG, "rdp_decrypt failed");
				return -1;
			}
		}
	}

	if (channelId == MCS_GLOBAL_CHANNEL_ID)
	{
		UINT16 pduLength, remain;
		if (!rdp_read_share_control_header(s, &pduLength, &remain, &pduType, &pduSource))
			return -1;

		client->settings->PduSource = pduSource;

		WLog_DBG(TAG, "Received %s", pdu_type_to_str(pduType));
		switch (pduType)
		{
			case PDU_TYPE_DATA:
				if (!peer_recv_data_pdu(client, s, pduLength))
					return -1;

				break;

			case PDU_TYPE_CONFIRM_ACTIVE:
				if (!rdp_server_accept_confirm_active(rdp, s, pduLength))
					return -1;

				break;

			case PDU_TYPE_FLOW_RESPONSE:
			case PDU_TYPE_FLOW_STOP:
			case PDU_TYPE_FLOW_TEST:
				if (!Stream_SafeSeek(s, remain))
					return -1;
				break;

			default:
				WLog_ERR(TAG, "Client sent pduType %" PRIu16 "", pduType);
				return -1;
		}
	}
	else if ((rdp->mcs->messageChannelId > 0) && (channelId == rdp->mcs->messageChannelId))
	{
		if (!rdp->settings->UseRdpSecurityLayer)
			if (!rdp_read_security_header(s, &securityFlags, NULL))
				return -1;

		return rdp_recv_message_channel_pdu(rdp, s, securityFlags);
	}
	else
	{
		if (!freerdp_channel_peer_process(client, s, channelId))
			return -1;
	}
	if (!tpkt_ensure_stream_consumed(s, length))
		return -1;

	return 0;
}

static int peer_recv_fastpath_pdu(freerdp_peer* client, wStream* s)
{
	rdpRdp* rdp;
	UINT16 length;
	rdpFastPath* fastpath;
	rdp = client->context->rdp;
	fastpath = rdp->fastpath;
	fastpath_read_header_rdp(fastpath, s, &length);

	if ((length == 0) || (length > Stream_GetRemainingLength(s)))
	{
		WLog_ERR(TAG, "incorrect FastPath PDU header length %" PRIu16 "", length);
		return -1;
	}

	if (fastpath->encryptionFlags & FASTPATH_OUTPUT_ENCRYPTED)
	{
		if (!rdp_decrypt(rdp, s, &length,
		                 (fastpath->encryptionFlags & FASTPATH_OUTPUT_SECURE_CHECKSUM)
		                     ? SEC_SECURE_CHECKSUM
		                     : 0))
			return -1;
	}

	rdp->inPackets++;

	return fastpath_recv_inputs(fastpath, s);
}

static int peer_recv_pdu(freerdp_peer* client, wStream* s)
{
	if (tpkt_verify_header(s))
		return peer_recv_tpkt_pdu(client, s);
	else
		return peer_recv_fastpath_pdu(client, s);
}

static int peer_recv_callback(rdpTransport* transport, wStream* s, void* extra)
{
	UINT32 SelectedProtocol;
	freerdp_peer* client = (freerdp_peer*)extra;
	rdpRdp* rdp = client->context->rdp;

	switch (rdp_get_state(rdp))
	{
		case CONNECTION_STATE_INITIAL:
			if (!rdp_server_accept_nego(rdp, s))
			{
				WLog_ERR(TAG, "%s: %s - rdp_server_accept_nego() fail", __FUNCTION__,
				         rdp_state_string(rdp_get_state(rdp)));
				return -1;
			}

			SelectedProtocol = nego_get_selected_protocol(rdp->nego);
			client->settings->NlaSecurity = (SelectedProtocol & PROTOCOL_HYBRID) ? TRUE : FALSE;
			client->settings->TlsSecurity = (SelectedProtocol & PROTOCOL_SSL) ? TRUE : FALSE;
			client->settings->RdpSecurity = (SelectedProtocol == PROTOCOL_RDP) ? TRUE : FALSE;

			if (SelectedProtocol & PROTOCOL_HYBRID)
			{
				SEC_WINNT_AUTH_IDENTITY* identity = nego_get_identity(rdp->nego);
				sspi_CopyAuthIdentity(&client->identity, identity);
				IFCALLRET(client->Logon, client->authenticated, client, &client->identity, TRUE);
				nego_free_nla(rdp->nego);
			}
			else
			{
				IFCALLRET(client->Logon, client->authenticated, client, &client->identity, FALSE);
			}

			break;

		case CONNECTION_STATE_NEGO:
			if (!rdp_server_accept_mcs_connect_initial(rdp, s))
			{
				WLog_ERR(TAG,
				         "%s: %s - "
				         "rdp_server_accept_mcs_connect_initial() fail",
				         __FUNCTION__, rdp_state_string(rdp_get_state(rdp)));
				return -1;
			}

			break;

		case CONNECTION_STATE_MCS_CONNECT:
			if (!rdp_server_accept_mcs_erect_domain_request(rdp, s))
			{
				WLog_ERR(TAG,
				         "%s: %s - "
				         "rdp_server_accept_mcs_erect_domain_request() fail",
				         __FUNCTION__, rdp_state_string(rdp_get_state(rdp)));
				return -1;
			}

			break;

		case CONNECTION_STATE_MCS_ERECT_DOMAIN:
			if (!rdp_server_accept_mcs_attach_user_request(rdp, s))
			{
				WLog_ERR(TAG,
				         "%s: %s - "
				         "rdp_server_accept_mcs_attach_user_request() fail",
				         __FUNCTION__, rdp_state_string(rdp_get_state(rdp)));
				return -1;
			}

			break;

		case CONNECTION_STATE_MCS_ATTACH_USER:
			if (!rdp_server_accept_mcs_channel_join_request(rdp, s))
			{
				WLog_ERR(TAG,
				         "%s: %s - "
				         "rdp_server_accept_mcs_channel_join_request() fail",
				         __FUNCTION__, rdp_state_string(rdp_get_state(rdp)));
				return -1;
			}

			break;

		case CONNECTION_STATE_RDP_SECURITY_COMMENCEMENT:
			if (rdp->settings->UseRdpSecurityLayer)
			{
				if (!rdp_server_establish_keys(rdp, s))
				{
					WLog_ERR(TAG,
					         "%s: %s - "
					         "rdp_server_establish_keys() fail",
					         __FUNCTION__, rdp_state_string(rdp_get_state(rdp)));
					return -1;
				}
			}

			rdp_server_transition_to_state(rdp, CONNECTION_STATE_SECURE_SETTINGS_EXCHANGE);

			if (Stream_GetRemainingLength(s) > 0)
				return peer_recv_callback(transport, s, extra);

			break;

		case CONNECTION_STATE_SECURE_SETTINGS_EXCHANGE:
			if (!rdp_recv_client_info(rdp, s))
			{
				WLog_ERR(TAG,
				         "%s: %s - "
				         "rdp_recv_client_info() fail",
				         __FUNCTION__, rdp_state_string(rdp_get_state(rdp)));
				return -1;
			}

			rdp_server_transition_to_state(rdp, CONNECTION_STATE_LICENSING);
			return peer_recv_callback(transport, NULL, extra);

		case CONNECTION_STATE_LICENSING:
		{
			LicenseCallbackResult res;

			if (!client->LicenseCallback)
			{
				WLog_ERR(TAG,
				         "%s: LicenseCallback has been removed, assuming "
				         "licensing is ok (please fix your app)",
				         __FUNCTION__);
				res = LICENSE_CB_COMPLETED;
			}
			else
				res = client->LicenseCallback(client, s);

			switch (res)
			{
				case LICENSE_CB_INTERNAL_ERROR:
					WLog_ERR(TAG,
					         "%s: %s - callback internal "
					         "error, aborting",
					         __FUNCTION__, rdp_state_string(rdp_get_state(rdp)));
					return -1;

				case LICENSE_CB_ABORT:
					return -1;

				case LICENSE_CB_IN_PROGRESS:
					break;

				case LICENSE_CB_COMPLETED:
					rdp_server_transition_to_state(rdp, CONNECTION_STATE_CAPABILITIES_EXCHANGE);
					return peer_recv_callback(transport, NULL, extra);

				default:
					WLog_ERR(TAG,
					         "%s: CONNECTION_STATE_LICENSING - unknown license callback "
					         "result %d",
					         __FUNCTION__, res);
					break;
			}

			break;
		}

		case CONNECTION_STATE_CAPABILITIES_EXCHANGE:
			if (!rdp->AwaitCapabilities)
			{
				if (client->Capabilities && !client->Capabilities(client))
					return -1;

				if (!rdp_send_demand_active(rdp))
				{
					WLog_ERR(TAG,
					         "%s: %s - "
					         "rdp_send_demand_active() fail",
					         __FUNCTION__, rdp_state_string(rdp_get_state(rdp)));
					return -1;
				}

				rdp->AwaitCapabilities = TRUE;

				if (s)
				{
					if (peer_recv_pdu(client, s) < 0)
					{
						WLog_ERR(TAG,
						         "%s: %s - "
						         "peer_recv_pdu() fail",
						         __FUNCTION__, rdp_state_string(rdp_get_state(rdp)));
						return -1;
					}
				}
			}
			else
			{
				/**
				 * During reactivation sequence the client might sent some input or channel data
				 * before receiving the Deactivate All PDU. We need to process them as usual.
				 */
				if (peer_recv_pdu(client, s) < 0)
				{
					WLog_ERR(TAG,
					         "%s: %s - "
					         "peer_recv_pdu() fail",
					         __FUNCTION__, rdp_state_string(rdp_get_state(rdp)));
					return -1;
				}
			}

			break;

		case CONNECTION_STATE_FINALIZATION:
			if (peer_recv_pdu(client, s) < 0)
			{
				WLog_ERR(TAG, "%s: %s - peer_recv_pdu() fail", __FUNCTION__,
				         rdp_state_string(rdp_get_state(rdp)));
				return -1;
			}

			break;

		case CONNECTION_STATE_ACTIVE:
			if (peer_recv_pdu(client, s) < 0)
			{
				WLog_ERR(TAG, "%s: %s - peer_recv_pdu() fail", __FUNCTION__,
				         rdp_state_string(rdp_get_state(rdp)));
				return -1;
			}

			break;

		default:
			WLog_ERR(TAG, "%s state %d", rdp_state_string(rdp_get_state(rdp)), rdp_get_state(rdp));
			return -1;
	}

	return 0;
}

static BOOL freerdp_peer_close(freerdp_peer* client)
{
	UINT32 SelectedProtocol;
	/** if negotiation has failed, we're not MCS connected. So don't
	 * 	send anything else, or some mstsc will consider that as an error
	 */
	SelectedProtocol = nego_get_selected_protocol(client->context->rdp->nego);

	if (SelectedProtocol & PROTOCOL_FAILED_NEGO)
		return TRUE;

	/**
	 * [MS-RDPBCGR] 1.3.1.4.2 User-Initiated Disconnection Sequence on Server
	 * The server first sends the client a Deactivate All PDU followed by an
	 * optional MCS Disconnect Provider Ultimatum PDU.
	 */
	if (!rdp_send_deactivate_all(client->context->rdp))
		return FALSE;

	if (freerdp_settings_get_bool(client->settings, FreeRDP_SupportErrorInfoPdu))
	{
		rdp_send_error_info(client->context->rdp);
	}

	return mcs_send_disconnect_provider_ultimatum(client->context->rdp->mcs);
}

static void freerdp_peer_disconnect(freerdp_peer* client)
{
	rdpTransport* transport = client->context->rdp->transport;
	transport_disconnect(transport);
}

static BOOL freerdp_peer_send_channel_data(freerdp_peer* client, UINT16 channelId, const BYTE* data,
                                           size_t size)
{
	return rdp_send_channel_data(client->context->rdp, channelId, data, size);
}

static BOOL freerdp_peer_send_server_redirection_pdu(
    freerdp_peer* peer, UINT32 sessionId, const char* targetNetAddress, const char* routingToken,
    const char* userName, const char* domain, const char* password, const char* targetFQDN,
    const char* targetNetBiosName, DWORD tsvUrlLength, const BYTE* tsvUrl,
    UINT32 targetNetAddressesCount, const char** targetNetAddresses)
{
	wStream* s = rdp_send_stream_pdu_init(peer->context->rdp);
	UINT16 length;
	UINT32 redirFlags;

	UINT32 targetNetAddressLength;
	UINT32 loadBalanceInfoLength;
	UINT32 userNameLength;
	UINT32 domainLength;
	UINT32 passwordLength;
	UINT32 targetFQDNLength;
	UINT32 targetNetBiosNameLength;
	UINT32 targetNetAddressesLength;
	UINT32* targetNetAddressesWLength;

	LPWSTR targetNetAddressW = NULL;
	LPWSTR userNameW = NULL;
	LPWSTR domainW = NULL;
	LPWSTR passwordW = NULL;
	LPWSTR targetFQDNW = NULL;
	LPWSTR targetNetBiosNameW = NULL;
	LPWSTR* targetNetAddressesW = NULL;

	length = 12; /* Flags (2) + length (2) + sessionId (4)  + redirection flags (4) */
	redirFlags = 0;

	if (targetNetAddress)
	{
		redirFlags |= LB_TARGET_NET_ADDRESS;

		ConvertToUnicode(CP_UTF8, 0, (LPCSTR)targetNetAddress, -1, &targetNetAddressW, 0);
		targetNetAddressLength = (strlen(targetNetAddress) + 1) * sizeof(WCHAR);

		length += 4 + targetNetAddressLength;
	}

	if (routingToken)
	{
		redirFlags |= LB_LOAD_BALANCE_INFO;
		loadBalanceInfoLength =
		    13 + strlen(routingToken) + 2; /* Add routing token prefix and suffix */
		length += 4 + loadBalanceInfoLength;
	}

	if (userName)
	{
		redirFlags |= LB_USERNAME;

		ConvertToUnicode(CP_UTF8, 0, (LPCSTR)userName, -1, &userNameW, 0);
		userNameLength = (strlen(userName) + 1) * sizeof(WCHAR);

		length += 4 + userNameLength;
	}

	if (domain)
	{
		redirFlags |= LB_DOMAIN;

		ConvertToUnicode(CP_UTF8, 0, (LPCSTR)domain, -1, &domainW, 0);
		domainLength = (strlen(domain) + 1) * sizeof(WCHAR);

		length += 4 + domainLength;
	}

	if (password)
	{
		redirFlags |= LB_PASSWORD;

		ConvertToUnicode(CP_UTF8, 0, (LPCSTR)password, -1, &passwordW, 0);
		passwordLength = (strlen(password) + 1) * sizeof(WCHAR);

		length += 4 + passwordLength;
	}

	if (targetFQDN)
	{
		redirFlags |= LB_TARGET_FQDN;

		ConvertToUnicode(CP_UTF8, 0, (LPCSTR)targetFQDN, -1, &targetFQDNW, 0);
		targetFQDNLength = (strlen(targetFQDN) + 1) * sizeof(WCHAR);

		length += 4 + targetFQDNLength;
	}

	if (targetNetBiosName)
	{
		redirFlags |= LB_TARGET_NETBIOS_NAME;

		ConvertToUnicode(CP_UTF8, 0, (LPCSTR)targetNetBiosName, -1, &targetNetBiosNameW, 0);
		targetNetBiosNameLength = (strlen(targetNetBiosName) + 1) * sizeof(WCHAR);

		length += 4 + targetNetBiosNameLength;
	}

	if (tsvUrl)
	{
		redirFlags |= LB_CLIENT_TSV_URL;
		length += 4 + tsvUrlLength;
	}

	if (targetNetAddresses)
	{
		UINT32 i;

		redirFlags |= LB_TARGET_NET_ADDRESSES;

		targetNetAddressesLength = 0;
		targetNetAddressesW = calloc(targetNetAddressesCount, sizeof(LPWSTR));
		targetNetAddressesWLength = calloc(targetNetAddressesCount, sizeof(UINT32));
		for (i = 0; i < targetNetAddressesCount; i++)
		{
			ConvertToUnicode(CP_UTF8, 0, (LPCSTR)targetNetAddresses[i], -1, &targetNetAddressesW[i],
			                 0);
			targetNetAddressesWLength[i] = (strlen(targetNetAddresses[i]) + 1) * sizeof(WCHAR);
			targetNetAddressesLength += 4 + targetNetAddressesWLength[i];
		}

		length += 4 + 4 + targetNetAddressesLength;
	}

	Stream_Write_UINT16(s, 0);
	Stream_Write_UINT16(s, SEC_REDIRECTION_PKT);
	Stream_Write_UINT16(s, length);

	if (!Stream_EnsureRemainingCapacity(s, length))
	{
		WLog_ERR(TAG, "Stream_EnsureRemainingCapacity failed!");
		goto fail;
	}

	if (sessionId)
		Stream_Write_UINT32(s, sessionId);
	else
		Stream_Write_UINT32(s, 0);

	Stream_Write_UINT32(s, redirFlags);

	if (redirFlags & LB_TARGET_NET_ADDRESS)
	{
		Stream_Write_UINT32(s, targetNetAddressLength);
		Stream_Write(s, targetNetAddressW, targetNetAddressLength);
		free(targetNetAddressW);
	}

	if (redirFlags & LB_LOAD_BALANCE_INFO)
	{
		Stream_Write_UINT32(s, loadBalanceInfoLength);
		Stream_Write(s, "Cookie: msts=", 13);
		Stream_Write(s, routingToken, strlen(routingToken));
		Stream_Write_UINT8(s, 0x0d);
		Stream_Write_UINT8(s, 0x0a);
	}

	if (redirFlags & LB_USERNAME)
	{
		Stream_Write_UINT32(s, userNameLength);
		Stream_Write(s, userNameW, userNameLength);
		free(userNameW);
	}

	if (redirFlags & LB_DOMAIN)
	{
		Stream_Write_UINT32(s, domainLength);
		Stream_Write(s, domainW, domainLength);
		free(domainW);
	}

	if (redirFlags & LB_PASSWORD)
	{
		Stream_Write_UINT32(s, passwordLength);
		Stream_Write(s, passwordW, passwordLength);
		free(passwordW);
	}

	if (redirFlags & LB_TARGET_FQDN)
	{
		Stream_Write_UINT32(s, targetFQDNLength);
		Stream_Write(s, targetFQDNW, targetFQDNLength);
		free(targetFQDNW);
	}

	if (redirFlags & LB_TARGET_NETBIOS_NAME)
	{
		Stream_Write_UINT32(s, targetNetBiosNameLength);
		Stream_Write(s, targetNetBiosNameW, targetNetBiosNameLength);
		free(targetNetBiosNameW);
	}

	if (redirFlags & LB_CLIENT_TSV_URL)
	{
		Stream_Write_UINT32(s, tsvUrlLength);
		Stream_Write(s, tsvUrl, tsvUrlLength);
	}

	if (redirFlags & LB_TARGET_NET_ADDRESSES)
	{
		UINT32 i;
		Stream_Write_UINT32(s, targetNetAddressesLength);
		Stream_Write_UINT32(s, targetNetAddressesCount);
		for (i = 0; i < targetNetAddressesCount; i++)
		{
			Stream_Write_UINT32(s, targetNetAddressesWLength[i]);
			Stream_Write(s, targetNetAddressesW[i], targetNetAddressesWLength[i]);
			free(targetNetAddressesW[i]);
		}
		free(targetNetAddressesW);
		free(targetNetAddressesWLength);
	}

	Stream_Write_UINT8(s, 0);
	rdp_send_pdu(peer->context->rdp, s, PDU_TYPE_SERVER_REDIRECTION, 0);

	return TRUE;

fail:
	free(targetNetAddressW);
	free(userNameW);
	free(domainW);
	free(passwordW);
	free(targetFQDNW);
	free(targetNetBiosNameW);
	free(targetNetAddressesWLength);

	if (targetNetAddressesCount > 0)
	{
		UINT32 i;
		for (i = 0; i < targetNetAddressesCount; i++)
			free(targetNetAddressesW[i]);
		free(targetNetAddressesW);
	}

	return FALSE;
}

static BOOL freerdp_peer_is_write_blocked(freerdp_peer* peer)
{
	rdpTransport* transport = peer->context->rdp->transport;
	return transport_is_write_blocked(transport);
}

static int freerdp_peer_drain_output_buffer(freerdp_peer* peer)
{
	rdpTransport* transport = peer->context->rdp->transport;
	return transport_drain_output_buffer(transport);
}

static BOOL freerdp_peer_has_more_to_read(freerdp_peer* peer)
{
	return peer->context->rdp->transport->haveMoreBytesToRead;
}

static LicenseCallbackResult freerdp_peer_nolicense(freerdp_peer* peer, wStream* s)
{
	rdpRdp* rdp = peer->context->rdp;

	if (!license_send_valid_client_error_packet(rdp))
	{
		WLog_ERR(TAG, "freerdp_peer_nolicense: license_send_valid_client_error_packet() failed");
		return LICENSE_CB_ABORT;
	}

	return LICENSE_CB_COMPLETED;
}

BOOL freerdp_peer_context_new(freerdp_peer* client)
{
	rdpRdp* rdp;
	rdpContext* context;
	BOOL ret = TRUE;

	if (!client)
		return FALSE;

	if (!(context = (rdpContext*)calloc(1, client->ContextSize)))
		goto fail_context;

	client->context = context;
	context->peer = client;
	context->ServerMode = TRUE;
	context->settings = client->settings;

	if (!(context->metrics = metrics_new(context)))
		goto fail_metrics;

	if (!(rdp = rdp_new(context)))
		goto fail_rdp;

	client->input = rdp->input;
	client->update = rdp->update;
	client->settings = rdp->settings;
	client->autodetect = rdp->autodetect;
	context->rdp = rdp;
	context->input = client->input;
	context->update = client->update;
	context->settings = client->settings;
	context->autodetect = client->autodetect;
	client->update->context = context;
	client->input->context = context;
	client->autodetect->context = context;
	update_register_server_callbacks(client->update);
	autodetect_register_server_callbacks(client->autodetect);

	if (!(context->errorDescription = calloc(1, 500)))
	{
		WLog_ERR(TAG, "calloc failed!");
		goto fail_error_description;
	}

	if (!transport_attach(rdp->transport, client->sockfd))
		goto fail_transport_attach;

	rdp->transport->ReceiveCallback = peer_recv_callback;
	rdp->transport->ReceiveExtra = client;
	transport_set_blocking_mode(rdp->transport, FALSE);
	client->IsWriteBlocked = freerdp_peer_is_write_blocked;
	client->DrainOutputBuffer = freerdp_peer_drain_output_buffer;
	client->HasMoreToRead = freerdp_peer_has_more_to_read;
	client->LicenseCallback = freerdp_peer_nolicense;
	IFCALLRET(client->ContextNew, ret, client, client->context);

	if (ret)
		return TRUE;

	WLog_ERR(TAG, "ContextNew callback failed");
fail_transport_attach:
	free(context->errorDescription);
fail_error_description:
	rdp_free(client->context->rdp);
fail_rdp:
	metrics_free(context->metrics);
fail_metrics:
	free(client->context);
fail_context:
	client->context = NULL;
	WLog_ERR(TAG, "Failed to create new peer context");
	return FALSE;
}

void freerdp_peer_context_free(freerdp_peer* client)
{
	IFCALL(client->ContextFree, client, client->context);

	if (client->context)
	{
		free(client->context->errorDescription);
		client->context->errorDescription = NULL;
		rdp_free(client->context->rdp);
		client->context->rdp = NULL;
		metrics_free(client->context->metrics);
		client->context->metrics = NULL;
		free(client->context);
		client->context = NULL;
	}
}

static const char* os_major_type_to_string(UINT16 osMajorType)
{
	switch (osMajorType)
	{
		case OSMAJORTYPE_UNSPECIFIED:
			return "Unspecified platform";
		case OSMAJORTYPE_WINDOWS:
			return "Windows platform";
		case OSMAJORTYPE_OS2:
			return "OS/2 platform";
		case OSMAJORTYPE_MACINTOSH:
			return "Macintosh platform";
		case OSMAJORTYPE_UNIX:
			return "UNIX platform";
		case OSMAJORTYPE_IOS:
			return "iOS platform";
		case OSMAJORTYPE_OSX:
			return "OS X platform";
		case OSMAJORTYPE_ANDROID:
			return "Android platform";
		case OSMAJORTYPE_CHROME_OS:
			return "Chrome OS platform";
	}

	return "Unknown platform";
}

const char* freerdp_peer_os_major_type_string(freerdp_peer* client)
{
	rdpContext* context;
	UINT16 osMajorType;

	WINPR_ASSERT(client);

	context = client->context;
	WINPR_ASSERT(context);
	WINPR_ASSERT(context->settings);

	osMajorType = freerdp_settings_get_uint32(context->settings, FreeRDP_OsMajorType);

	return os_major_type_to_string(osMajorType);
}

static const char* os_minor_type_to_string(UINT16 osMinorType)
{
	switch (osMinorType)
	{
		case OSMINORTYPE_UNSPECIFIED:
			return "Unspecified version";
		case OSMINORTYPE_WINDOWS_31X:
			return "Windows 3.1x";
		case OSMINORTYPE_WINDOWS_95:
			return "Windows 95";
		case OSMINORTYPE_WINDOWS_NT:
			return "Windows NT";
		case OSMINORTYPE_OS2_V21:
			return "OS/2 2.1";
		case OSMINORTYPE_POWER_PC:
			return "PowerPC";
		case OSMINORTYPE_MACINTOSH:
			return "Macintosh";
		case OSMINORTYPE_NATIVE_XSERVER:
			return "Native X Server";
		case OSMINORTYPE_PSEUDO_XSERVER:
			return "Pseudo X Server";
		case OSMINORTYPE_WINDOWS_RT:
			return "Windows RT";
	}

	return "Unknown version";
}

const char* freerdp_peer_os_minor_type_string(freerdp_peer* client)
{
	rdpContext* context;
	UINT16 osMinorType;

	WINPR_ASSERT(client);

	context = client->context;
	WINPR_ASSERT(context);
	WINPR_ASSERT(context->settings);

	osMinorType = freerdp_settings_get_uint32(context->settings, FreeRDP_OsMinorType);

	return os_minor_type_to_string(osMinorType);
}

freerdp_peer* freerdp_peer_new(int sockfd)
{
	UINT32 option_value;
	socklen_t option_len;
	freerdp_peer* client;
	client = (freerdp_peer*)calloc(1, sizeof(freerdp_peer));

	if (!client)
		return NULL;

	option_value = TRUE;
	option_len = sizeof(option_value);
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void*)&option_value, option_len);

	if (client)
	{
		client->sockfd = sockfd;
		client->ContextSize = sizeof(rdpContext);
		client->Initialize = freerdp_peer_initialize;
		client->GetFileDescriptor = freerdp_peer_get_fds;
		client->GetEventHandle = freerdp_peer_get_event_handle;
		client->GetEventHandles = freerdp_peer_get_event_handles;
		client->CheckFileDescriptor = freerdp_peer_check_fds;
		client->Close = freerdp_peer_close;
		client->Disconnect = freerdp_peer_disconnect;
		client->SendChannelData = freerdp_peer_send_channel_data;
		client->SendServerRedirection = freerdp_peer_send_server_redirection_pdu;
		client->IsWriteBlocked = freerdp_peer_is_write_blocked;
		client->DrainOutputBuffer = freerdp_peer_drain_output_buffer;
		client->HasMoreToRead = freerdp_peer_has_more_to_read;
		client->VirtualChannelOpen = freerdp_peer_virtual_channel_open;
		client->VirtualChannelClose = freerdp_peer_virtual_channel_close;
		client->VirtualChannelWrite = freerdp_peer_virtual_channel_write;
		client->VirtualChannelRead = NULL; /* must be defined by server application */
		client->VirtualChannelGetData = freerdp_peer_virtual_channel_get_data;
		client->VirtualChannelSetData = freerdp_peer_virtual_channel_set_data;
	}

	return client;
}

void freerdp_peer_free(freerdp_peer* client)
{
	if (!client)
		return;

	free(client);
}
