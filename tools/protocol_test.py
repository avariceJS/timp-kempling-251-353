#!/usr/bin/env python3
# Протокол-тест qt_tcp_server: приветствие, лимит 4, список, /finish.
import socket
import time
import sys

HOST = "127.0.0.1"
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 5555


def recv_available(sock, wait=0.4):
    sock.settimeout(wait)
    chunks = []
    try:
        while True:
            data = sock.recv(4096)
            if not data:
                break
            chunks.append(data)
    except socket.timeout:
        pass
    return b"".join(chunks).decode("utf-8", errors="replace")


def connect():
    s = socket.create_connection((HOST, PORT), timeout=3)
    s.settimeout(0.5)
    return s


def send(sock, text):
    sock.sendall((text + "\n").encode("utf-8"))


def main():
    print("=== Подключаем 4 клиентов ===")
    clients = []
    for i in range(4):
        s = connect()
        msg = recv_available(s)
        print(f"[client {i+1} greeting]\n{msg}")
        if "Добро пожаловать" not in msg:
            raise SystemExit(f"Нет приветствия у клиента {i+1}")
        if "Подключено клиентов:" not in msg:
            raise SystemExit(f"Нет рассылки числа клиентов у клиента {i+1}")
        clients.append(s)

    print("=== Пятый клиент должен получить «занят» и отключение ===")
    busy = connect()
    busy_msg = recv_available(busy, wait=0.8)
    print(f"[busy]\n{busy_msg}")
    if "занят" not in busy_msg.lower() and "занят" not in busy_msg:
        # already contains Russian
        pass
    if "занят" not in busy_msg:
        raise SystemExit("Пятый клиент не получил сообщение о занятости")
    leftover = recv_available(busy, wait=0.8)
    try:
        busy.sendall(b"ping\n")
        more = recv_available(busy, wait=0.4)
        leftover += more
    except OSError:
        pass
    print("busy leftover:", repr(leftover))
    busy.close()

    print("=== Редактирование списка ===")
    send(clients[0], "/add молоко")
    send(clients[1], "/add хлеб")
    time.sleep(0.3)
    print("[after add]\n", recv_available(clients[2], wait=0.5))
    send(clients[2], "/remove молоко")
    time.sleep(0.2)
    send(clients[3], "/add масло")
    time.sleep(0.2)

    print("=== Все отправляют /finish ===")
    for s in clients:
        send(s, "/finish")
    time.sleep(0.4)
    final = recv_available(clients[0], wait=0.6)
    print("[final to client 1]\n", final)
    if "Итоговый список" not in final and "Редактирование завершено" not in final:
        # maybe in another client buffer
        final2 = recv_available(clients[1], wait=0.4)
        print("[final to client 2]\n", final2)
        if "Итоговый список" not in final2 and "Редактирование завершено" not in final2:
            raise SystemExit("Итоговый список не разослан")

    print("OK: протокол работает")
    for s in clients:
        s.close()


if __name__ == "__main__":
    main()
