#
# FreeRDP: A Remote Desktop Protocol Implementation
# FreeRDP Proxy Server Capture Module
#
# Copyright 2019 Kobi Mizrachi <kmizrachi18@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# deactivated: does not work

#set(PLUGIN_NAME "proxy-capture-plugin")
#
#add_library(${PLUGIN_NAME} MODULE
#	cap_main.c
#	cap_config.c
#	cap_config.h
#	cap_protocol.c
#	cap_protocol.h
#)
#
#set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")
#set_target_properties(${PLUGIN_NAME} PROPERTIES NO_SONAME 1)
#set_target_properties(${PLUGIN_NAME} PROPERTIES
#LIBRARY_OUTPUT_DIRECTORY "${FREERDP_PROXY_PLUGINDIR}")
