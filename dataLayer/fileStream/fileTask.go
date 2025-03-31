package fileStream

import (
	"FuckRDP/dataLayer"
	"FuckRDP/grdp/glog"
	"FuckRDP/grdp/protocol/pdu"
	"hash/crc32"
	"io"
	"os"
	"path/filepath"
	"runtime"
)

type FileTask struct {
	head pdu.AkuaFileTransferHead
	file *os.File
	stop chan struct{}
}

func NewFileTask(srcFilePath, targetFilePath string) (*FileTask, error) {
	fileName := filepath.Base(targetFilePath)
	dirPath := filepath.Dir(targetFilePath)
	file, err := os.Open(srcFilePath)
	if err != nil {
		return nil, err
	}
	return &FileTask{
		file: file,
		head: pdu.NewAkuaFileTransferHead(fileName, dirPath),
		stop: make(chan struct{}, 1),
	}, nil
}

func (task *FileTask) StartTransfer() {
	fileInfo, err := task.file.Stat()
	if err != nil {
		glog.Errorf("[FileTransfer]: %v %v", task, err)
	}
	startPDU := pdu.NewAkuaFileTransferStart(task.head, uint32(fileInfo.Size()))
	dataLayer.LowPriorityInput(startPDU)
	taskJoin(task)
	go func(task *FileTask) {
		var (
			buffer = make([]byte, 2*1024)
			num    = 0
			index  = 0
			err    error
		)

		for {
			select {
			case <-task.stop:
				return
			default:
				num, err = task.file.Read(buffer)
				if num != 0 {
					newArray := make([]byte, num)
					copy(newArray, buffer[:num])
					packetPDU := pdu.NewAkuaFileTransferPacket(task.head, uint32(index), newArray)
					dataLayer.LowPriorityInput(packetPDU)
					runtime.Gosched()
				}
				index += num
				if err != nil && err.Error() == "EOF" {
					task.verify()
					return
				}
			}
		}
	}(task)
}

func (task *FileTask) Abort() {
	abortPDU := pdu.NewAkuaFileTransferAbort(task.head)
	dataLayer.LowPriorityInput(abortPDU)
	taskRemove(task)
	task.stop <- struct{}{}
	close(task.stop)
}

func (task *FileTask) verify() {
	_, _ = task.file.Seek(0, io.SeekStart)
	hash := crc32.NewIEEE()
	_, _ = io.Copy(hash, task.file)
	fileInfo, _ := task.file.Stat()
	verifyPDU := pdu.NewAkuaFileTransferVerify(task.head, uint32(fileInfo.Size()), hash.Sum32())
	glog.Debugf("upload verify Crc32: %d", hash.Sum32())
	dataLayer.LowPriorityInput(verifyPDU)
	task.stop <- struct{}{}
	close(task.stop)
}
