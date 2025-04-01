package cmdStream

import (
	"bufio"
	"fmt"
	"log"
	"net/http"
	"os/exec"
	"strings"
	"sync"
	"testing"

	"github.com/gorilla/websocket"
)

func handleTerminal(w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Println("WebSocket 升级失败:", err)
		return
	}
	defer conn.Close()

	cmd := exec.Command("powershell.exe")
	stdin, err := cmd.StdinPipe()
	if err != nil {
		log.Println("获取标准输入管道失败:", err)
		return
	}
	stdout, err := cmd.StdoutPipe()
	if err != nil {
		log.Println("获取标准输出管道失败:", err)
		return
	}
	stderr, err := cmd.StderrPipe()
	if err != nil {
		log.Println("获取标准错误管道失败:", err)
		return
	}

	if err := cmd.Start(); err != nil {
		log.Println("启动 PowerShell 失败:", err)
		return
	}

	var wg sync.WaitGroup
	wg.Add(2)

	go func() {
		defer wg.Done()
		scanner := bufio.NewScanner(stdout)
		for scanner.Scan() {
			line := scanner.Bytes()
			if err := conn.WriteMessage(websocket.BinaryMessage, line); err != nil {
				log.Println("发送消息到客户端失败:", err)
				return
			}
		}
		if err := scanner.Err(); err != nil {
			log.Println("读取标准输出失败:", err)
		}
	}()

	go func() {
		defer wg.Done()
		scanner := bufio.NewScanner(stderr)
		for scanner.Scan() {
			line := scanner.Bytes()
			if err := conn.WriteMessage(websocket.BinaryMessage, line); err != nil {
				log.Println("发送错误消息到客户端失败:", err)
				return
			}
		}
		if err := scanner.Err(); err != nil {
			log.Println("读取标准错误输出失败:", err)
		}
	}()

	for {
		_, message, err := conn.ReadMessage()
		if err != nil {
			log.Println("读取客户端消息失败:", err)
			break
		}
		command := strings.TrimSpace(string(message))
		if command == "exit" {
			break
		}
		if _, err := fmt.Fprintf(stdin, "%s\n", command); err != nil {
			log.Println("发送命令到 PowerShell 失败:", err)
			break
		}
	}

	if err := cmd.Process.Kill(); err != nil {
		log.Println("终止 PowerShell 进程失败:", err)
	}
	wg.Wait()
}

func TestCmd(t *testing.T) {
	http.HandleFunc("/terminal", handleTerminal)
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		log.Println("test")
		http.ServeFile(w, r, "cmd.html")
	})
	log.Println("服务器启动，监听端口 :8080")
	log.Fatal(http.ListenAndServe(":8080", nil))
}
