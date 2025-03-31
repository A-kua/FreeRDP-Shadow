package controlStream

import (
	"FuckRDP/dataLayer"
	"FuckRDP/grdp/protocol/pdu"
)

const EVENT_CONTROL_STATECHANGE = "event_control_statechange"

func SendEnableControl() {
	event := pdu.NewAkuaControlStatePacket(true)
	dataLayer.HighPriorityInput(event)
	dataLayer.PublishEvent(EVENT_CONTROL_STATECHANGE, "隐匿控制状态：", "开")
}
func SendDisableControl() {
	event := pdu.NewAkuaControlStatePacket(false)
	dataLayer.HighPriorityInput(event)
	dataLayer.PublishEvent(EVENT_CONTROL_STATECHANGE, "隐匿控制状态：", "关")
}
