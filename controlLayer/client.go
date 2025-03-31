package controlLayer

import (
	"FuckRDP/common"
	"FuckRDP/dataLayer"
	"FuckRDP/grdp/core"
	"FuckRDP/grdp/glog"
	"FuckRDP/grdp/protocol/nla"
	"FuckRDP/grdp/protocol/pdu"
	"FuckRDP/grdp/protocol/sec"
	"FuckRDP/grdp/protocol/t125"
	"FuckRDP/grdp/protocol/tpkt"
	"FuckRDP/grdp/protocol/x224"
	"net"
)

type Client struct {
	tpkt                             *tpkt.TPKT      // TPKT协议层
	x224                             *x224.X224      // X224协议层
	mcs                              *t125.MCSClient // MCS协议层
	sec                              *sec.Client     // 安全层
	pdu                              *pdu.Client     // PDU协议层
	host, domain, username, password string
}

func NewClient(host, domain, username, password string) *Client {
	return &Client{
		host:     host,
		domain:   domain,
		username: username,
		password: password,
	}
}

func (client *Client) Login() error {
	conn, err := net.Dial("tcp", client.host)
	if err != nil {
		glog.Errorf("连接至%s失败：%v", client.host, err)
		return err
	}

	// 初始化协议栈
	client.initProtocolStack(conn, client.domain, client.username, client.password)

	// 建立X224连接
	err = client.x224.Connect()
	if err != nil {
		glog.Errorf("建立X224连接失败：%v", err)
		return err
	}
	return nil
}

// initProtocolStack 初始化RDP协议栈
func (client *Client) initProtocolStack(conn net.Conn, domain, username, password string) {
	// 创建协议层实例
	client.tpkt = tpkt.New(core.NewSocketLayer(conn), nla.NewNTLMv2(domain, username, password))
	client.x224 = x224.New(client.tpkt)
	client.mcs = t125.NewMCSClient(client.x224)
	client.sec = sec.NewClient(client.mcs)
	client.pdu = pdu.NewClient(client.sec)

	client.mcs.SetClientDesktop(uint16(1920), uint16(1080))

	// 设置认证信息
	client.sec.SetDomain(domain)
	client.sec.SetUsername(username)
	client.sec.SetPassword(password)

	// 配置协议层关联
	client.tpkt.SetFastPathListener(client.sec)
	client.sec.SetFastPathListener(client.pdu)

	client.sec.SetChannelSender(client.mcs)
}

// AddEventHandler 设置PDU事件处理器
func (client *Client) AddEventHandler(event, listener interface{}) {
	client.pdu.On(event, listener)
}

func login(client *Client, flinger *common.SurfaceFlinger) {
	err := client.Login()
	if err != nil {
		glog.Errorf("Login error: %v", err)
	}
	dataLayer.LooperStart(client.pdu)
	client.AddEventHandler("bitmap", func(rectangles []pdu.BitmapData) {
		for _, rectangle := range rectangles {
			data := rectangle.BitmapDataStream
			if rectangle.IsCompress() {
				data = common.BitmapDecompress(&rectangle)
			}
			dataLayer.BitmapInput(data, int(rectangle.DestLeft), int(rectangle.DestTop), int(rectangle.Width), int(rectangle.Height), common.Bpp(rectangle.BitsPerPixel))
		}
		flinger.TickOnce()
	})
}
