package common

import "container/list"

// InfiniteChannel 结构体用于实现无限缓冲区通道
type InfiniteChannel struct {
	in     chan interface{}
	out    chan interface{}
	buffer *list.List
}

// NewInfiniteChannel 创建一个新的无限缓冲区通道
func NewInfiniteChannel() *InfiniteChannel {
	uc := &InfiniteChannel{
		in:     make(chan interface{}),
		out:    make(chan interface{}),
		buffer: list.New(),
	}
	go uc.background()
	return uc
}

func (uc *InfiniteChannel) background() {
	defer close(uc.out)
	for {
		var out chan interface{}
		var front interface{}
		if uc.buffer.Len() > 0 {
			out = uc.out
			front = uc.buffer.Front().Value
		}
		select {
		case elem, ok := <-uc.in:
			if !ok {
				// 输入通道关闭，将缓冲区元素发送完后退出
				for uc.buffer.Len() > 0 {
					uc.out <- uc.buffer.Remove(uc.buffer.Front())
				}
				return
			}
			uc.buffer.PushBack(elem)
		case out <- front:
			uc.buffer.Remove(uc.buffer.Front())
		}
	}
}

func (uc *InfiniteChannel) Out() <-chan interface{} {
	return uc.out
}

func (uc *InfiniteChannel) In() chan<- interface{} {
	return uc.in
}
