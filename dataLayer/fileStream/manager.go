package fileStream

import (
	"FuckRDP/dataLayer"
	"FuckRDP/grdp/glog"
	"FuckRDP/grdp/protocol/pdu"
	"bytes"
	"fmt"
	"github.com/lunixbochs/struc"
	"hash/crc32"
)

const EVENT_FILETASK_JOIN = "event_filetask_join"
const EVENT_FILETASK_REMOVE = "event_filetask_remove"
const EVENT_FILETASK_VERIFY = "event_filetask_verify"

var files = make(map[string]*FileTask)

func init() {
	dataLayer.RegisterHandler(0x02, verifyBack)
}

func taskJoin(task *FileTask) {
	files[task.head.Key()] = task
	dataLayer.PublishEvent(EVENT_FILETASK_JOIN, fmt.Sprintf("%s/%s", task.head.FilePath, task.head.FileName), "上传开始")
}
func taskRemove(task *FileTask) {
	delete(files, task.head.Key())
	dataLayer.PublishEvent(EVENT_FILETASK_REMOVE, fmt.Sprintf("%s/%s", task.head.FilePath, task.head.FileName), "上传中止")
}

type AkuaFileVerifyBody struct {
	State uint8  `struc:"little"`
	Crc32 uint32 `struc:"big"`
}

func verifyBack(data []byte) {
	var headBuf, bodyBuf = bytes.NewBuffer(data[:len(data)-5]), bytes.NewBuffer(data[len(data)-5:])
	verifyHead := &pdu.AkuaFileTransferHead{}
	verifyBody := &AkuaFileVerifyBody{}

	err := struc.Unpack(headBuf, verifyHead)
	if err != nil {
		glog.Errorf("verifyBack Head %v %v\n", err, data[:len(data)-5])
	} else {
		glog.Debugf("verifyBack Head %d %d %s %s\n", verifyHead.FileNameLength, verifyHead.FilePathLength, verifyHead.FileName, verifyHead.FilePath)
	}

	err = struc.Unpack(bodyBuf, verifyBody)
	if err != nil {
		glog.Errorf("verifyBack Body %v\n", err)
	} else {
		glog.Debugf("verifyBack Body %d %d\n", verifyBody.Crc32, verifyBody.State)
	}
	hash := crc32.NewIEEE()
	_, _ = hash.Write(data[:len(data)-4])
	if hash.Sum32() == verifyBody.Crc32 {
		switch verifyBody.State {
		case 255:
			dataLayer.PublishEvent(EVENT_FILETASK_VERIFY, fmt.Sprintf("%s/%s", verifyHead.FilePath, verifyHead.FileName), "上传成功")
		case 0:
			fallthrough
		default:
			dataLayer.PublishEvent(EVENT_FILETASK_VERIFY, fmt.Sprintf("%s/%s", verifyHead.FilePath, verifyHead.FileName), "服务端错误")
		}
	} else {
		glog.Infof("verify %s %s fail: crc32 is %d.", verifyHead.FileName, verifyHead.FilePath, hash.Sum32())
		dataLayer.PublishEvent(EVENT_FILETASK_VERIFY, fmt.Sprintf("%s/%s", verifyHead.FilePath, verifyHead.FileName), "验证信息传输失败")
	}
}
