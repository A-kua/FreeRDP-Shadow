package common

import (
	"fmt"
	"testing"
)

func TestInfiniteChannel(t *testing.T) {
	infiniteChannel := NewInfiniteChannel()

	go func() {
		for i := 0; i < 5; i++ {
			infiniteChannel.In() <- i
		}
		close(infiniteChannel.In())
	}()

	for elem := range infiniteChannel.Out() {
		fmt.Println(elem)
	}
}
