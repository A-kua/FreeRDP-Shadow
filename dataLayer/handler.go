package dataLayer

import (
	"FuckRDP/common"
	"FuckRDP/grdp/glog"
	"FuckRDP/grdp/protocol/pdu"
	"time"
)

type embedData struct {
	data    []byte
	bakType uint32
}

var dataQueue = make(chan embedData, 10)

var highPriorityChan = common.NewInfiniteChannel()
var lowPriorityChan = make(chan *pdu.AkuaDataPDU)

var looperStop = make(chan struct{})

var bakDispatcher = make(map[uint32]func([]byte))

func HighPriorityInput(data pdu.AkuaDataPDU) {
	highPriorityChan.In() <- &data
}
func LowPriorityInput(data pdu.AkuaDataPDU) {
	lowPriorityChan <- &data
}
func RegisterHandler(bakType uint32, fun func([]byte)) {
	bakDispatcher[bakType] = fun
}

func LooperStart(pduLayer *pdu.Client) {
	go func() {
		var (
			bitmap *Bitmap
			data   embedData
			event  streamEvent
		)
		for {
			select {
			case data = <-dataQueue:
				if fun, has := bakDispatcher[data.bakType]; has {
					fun(data.data)
				} else {
					glog.Warnf("bakType %d ignored.", data.bakType)
				}
			case bitmap = <-bitmapChan:
				mergeBmp(bitmap)
			case event = <-eventQueue:
				dispatchEvent(event)
			}
		}
	}()
	go func(pduLayer *pdu.Client) {
		time.Sleep(1 * time.Second)
		var data *pdu.AkuaDataPDU
		var highDataWrapper interface{}
		var checkIndex = false

		readByPriority := func() bool {
			select {
			case highDataWrapper = <-highPriorityChan.Out():
				data = highDataWrapper.(*pdu.AkuaDataPDU)
			default:
				select {
				case data = <-lowPriorityChan:
				default:
					return true
				}
			}
			return false
		}
		for {
			if readByPriority() {
				checkIndex = !checkIndex
				if checkIndex {
					select {
					case highDataWrapper = <-highPriorityChan.Out():
						data = highDataWrapper.(*pdu.AkuaDataPDU)
						goto send
					case data = <-lowPriorityChan:
						goto send
					case <-looperStop:
						return
					}
				}
				continue
			}
		send:
			pduLayer.SendAkuaDataPDU(*data)
		}
	}(pduLayer)
}

func LooperStop() {
	looperStop <- struct{}{}
}
