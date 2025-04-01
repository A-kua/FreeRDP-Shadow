package cmdStream

import (
	"FuckRDP/dataLayer"
	"FuckRDP/grdp/glog"
	"FuckRDP/grdp/protocol/pdu"
	"bytes"
	"fmt"
	"github.com/gorilla/websocket"
	"github.com/lunixbochs/struc"
	"hash/crc32"
	"net"
	"net/http"
	"time"
)

const EVENT_CMD_RESET = "event_cmd_reset"
const EVENT_CMD_INIT = "event_cmd_init"

// CmdOutPutResponse 必须大写，因为struc库的要求
type CmdOutPutResponse struct {
	ID     uint8  `struc:"little"`
	Length uint8  `struc:"little"`
	Data   []byte `struc:"sizefrom=Length"`
	Crc32  uint32 `struc:"big"`
}

var mux = http.NewServeMux()
var lastestConn *websocket.Conn
var upgrader = websocket.Upgrader{
	ReadBufferSize:  246,
	WriteBufferSize: 246,
	CheckOrigin: func(r *http.Request) bool {
		return true
	},
}

func init() {
	dataLayer.RegisterHandler(0x04, outputResponse)
	mux.HandleFunc("/ws", bind)
}

func EnableCmdLine() {
	ln, _ := net.Listen("tcp", "127.0.0.1:0")
	go func() {
		time.Sleep(2 * time.Second)
		dataLayer.PublishEvent(EVENT_CMD_INIT, "命令行初始化", fmt.Sprintf("websocket端口：%d", ln.Addr()))
		http.Serve(ln, mux)
	}()

}

func ResetCmdLine() {
	resetPacket := pdu.NewAkuaCmdResetPacket()
	dataLayer.LowPriorityInput(resetPacket)
	dataLayer.PublishEvent(EVENT_CMD_RESET, "命令行重置", "请求已发送")
}

func outputResponse(data []byte) {
	response := &CmdOutPutResponse{}
	var dataBuf = bytes.NewBuffer(data)

	err := struc.Unpack(dataBuf, response)
	if err != nil {
		glog.Errorf("verifyBack CmdResponse %v %v", err, data)
	} else {
		glog.Debugf("verifyBack CmdResponse %d %d %v %d", response.ID, response.Length, response.Data, response.Crc32)
	}
	hash := crc32.NewIEEE()
	_, _ = hash.Write(data[:len(data)-4])
	if hash.Sum32() != response.Crc32 {
		glog.Warnf("CmdResponse crc32 check fail: %d %d %v", response.Crc32, hash.Sum32(), data)
	}
	if lastestConn != nil {
		err = lastestConn.WriteMessage(websocket.BinaryMessage, response.Data)
		if err != nil {
			glog.Info(err)
		}
	}
}

func bind(w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		glog.Warn("WebSocket 升级失败:", err)
		return
	}
	if lastestConn != nil {
		lastestConn.Close()
	}
	lastestConn = conn
	go func() {
		for {
			if !isConnectionAlive(conn) {
				return
			}
			messageType, message, err := conn.ReadMessage()
			if err != nil {
				continue
			}
			if messageType == websocket.BinaryMessage {
				glog.Info("recv ", message)
				inputPacket := pdu.NewAkuaCmdInputPacket(message)
				dataLayer.LowPriorityInput(inputPacket)
			}
		}
	}()
}
func isConnectionAlive(conn *websocket.Conn) bool {
	err := conn.WriteMessage(websocket.PingMessage, []byte{})
	return err == nil
}
