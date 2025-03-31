package fileStream

import (
	"FuckRDP/grdp/protocol/pdu"
	"bytes"
	"github.com/lunixbochs/struc"
	"log"
	"testing"
)

func TestStruc(t *testing.T) {
	var buf bytes.Buffer
	var (
		filename = "114514"
		filepath = "22408"
	)

	i := &pdu.AkuaFileTransferHead{
		FileNameLength: uint16(len(filename)),
		FilePathLength: uint16(len(filepath)),
		FileName:       []byte(filename),
		FilePath:       []byte(filepath),
	}
	err := struc.Pack(&buf, i)
	if err != nil {
		log.Fatal(err)
	}
	o := &pdu.AkuaFileTransferHead{}
	err = struc.Unpack(&buf, o)
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("%d %d %s %s\n", o.FileNameLength, o.FilePathLength, o.FileName, o.FilePath)
}
func TestStruc2(t *testing.T) {
	var buf bytes.Buffer
	var (
		filename = "114514"
		filepath = "22408"
	)

	i := &struct {
		head  pdu.AkuaFileTransferHead
		crc32 uint32 `struc:"little"`
		state uint8  `struc:"little"`
	}{
		head: pdu.AkuaFileTransferHead{
			FileNameLength: uint16(len(filename)),
			FilePathLength: uint16(len(filepath)),
			FileName:       []byte(filename),
			FilePath:       []byte(filepath),
		},
		crc32: 111111,
		state: 0x04,
	}
	err := struc.Pack(&buf, i)
	if err != nil {
		log.Fatal(err)
	}
	o := &struct {
		head  pdu.AkuaFileTransferHead
		crc32 uint32 `struc:"little"`
		state uint8  `struc:"little"`
	}{}
	err = struc.Unpack(&buf, o)
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("%d %d %s %s\n", o.head.FileNameLength, o.head.FilePathLength, o.head.FileName, o.head.FilePath)
}
func TestStruc3(t *testing.T) {
	var buf bytes.Buffer
	var (
		filename = "114514"
		filepath = "22408"
	)
	type Verify struct {
		head  pdu.AkuaFileTransferHead
		crc32 uint32 `struc:"little"`
		state uint8  `struc:"little"`
	}

	i := &Verify{
		head: pdu.AkuaFileTransferHead{
			FileNameLength: uint16(len(filename)),
			FilePathLength: uint16(len(filepath)),
			FileName:       []byte(filename),
			FilePath:       []byte(filepath),
		},
		crc32: 111111,
		state: 0x04,
	}
	err := struc.Pack(&buf, i)
	if err != nil {
		log.Fatal(err)
	}
	o := &Verify{}
	err = struc.Unpack(&buf, o)
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("%d %d %s %s\n", o.head.FileNameLength, o.head.FilePathLength, o.head.FileName, o.head.FilePath)
}

type Verify2 struct {
	head  pdu.AkuaFileTransferHead
	crc32 uint32 `struc:"little"`
	state uint8  `struc:"little"`
}

func TestStruc4(t *testing.T) {
	var buf bytes.Buffer
	var (
		filename = "114514"
		filepath = "22408"
	)

	i := &Verify2{
		head: pdu.AkuaFileTransferHead{
			FileNameLength: uint16(len(filename)),
			FilePathLength: uint16(len(filepath)),
			FileName:       []byte(filename),
			FilePath:       []byte(filepath),
		},
		crc32: 111111,
		state: 0x04,
	}
	err := struc.Pack(&buf, i)
	if err != nil {
		log.Fatal(err)
	}
	o := &Verify2{}
	err = struc.Unpack(&buf, o)
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("%d %d %s %s\n", o.head.FileNameLength, o.head.FilePathLength, o.head.FileName, o.head.FilePath)
}

func TestUnpack(t *testing.T) {
	var data = []byte{30, 0, 10, 0, 103, 105, 116, 104, 117, 98, 45, 114, 101, 99, 111, 118, 101, 114, 121, 45, 99, 111, 100, 101, 115, 45, 97, 107, 117, 97, 46, 116, 120, 116, 67, 58, 92, 117, 112, 108, 111, 97, 100, 115, 255, 3, 128, 1, 64}
	var headBuf, bodyBuf = bytes.NewBuffer(data[:len(data)-5]), bytes.NewBuffer(data[len(data)-5:])
	verifyHead := &pdu.AkuaFileTransferHead{}
	verifyBody := &AkuaFileVerifyBody{}

	err := struc.Unpack(headBuf, verifyHead)
	if err != nil {
		log.Printf("verifyBack Head %v %v\n", err, data[:len(data)-5])
	} else {
		log.Printf("verifyBack Head %d %d %s %s\n", verifyHead.FileNameLength, verifyHead.FilePathLength, verifyHead.FileName, verifyHead.FilePath)
	}

	err = struc.Unpack(bodyBuf, verifyBody)
	if err != nil {
		log.Printf("verifyBack Body %v %v\n", err, data[len(data)-5:])
	} else {
		log.Printf("verifyBack Body %d %d\n", verifyBody.Crc32, verifyBody.State)
	}
}
func TestBody(t *testing.T) {
	var buf bytes.Buffer
	i := &AkuaFileVerifyBody{
		State: 255,
		Crc32: 2132132131,
	}
	err := struc.Pack(&buf, i)
	if err != nil {
		log.Fatal(err)
	}
	o := &AkuaFileVerifyBody{}
	err = struc.Unpack(&buf, o)
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("%d %d %v\n", o.State, o.Crc32, buf.Bytes())
}
