#!/usr/bin/env bash

#./build/$2/proteip.server $1 > server.txt &
#RUNNING_PID=$!
ADDRESS="127 0 0 1"
PORT=5000
BUILD_DIRECTORY=""
CLIENTW=5
CLIENTN=10
TEST_DIR="tests_multithread/"
COMMAND_FILE="test.txt"
HELP_TEXT="
Usage: %s: [-a server_address] [-p server_port] [-N number_of_clients]
[-n number_of_waves] [-t test_log_directory] [-B binary_directory] [-c commands_file]

-a - IP Адрес сервера через точки (Строку парсит getopts).
-p - Порт сервера
-N - Число клиентов, которые пытаются подключиться
-n - Число волн клиентов, которые пытаются подключиться
-t - Директория для логов скрипта
-B - Директория с proteip.server
-c - Файл в котором записан набор команд, завершающийся командой quit/exit

Описание:
Скрипт эмулирует n*N пользователей, которые выполняют набор операций в клиенте.
Скрипт созда для проверки работы мультипоточности сервера, как побочный эффект,
с помощью скрипта можно эмулировать любые команды клиенту. Скрипту необходимо 
указать директорию с бинарником.
Команды в файле, путь к которому указывается через -t, соответствуют командам в клиенте.

Пример использования:
./multithread.sh -a 127.0.0.1 -p 8888 -N 10 -n 8 -t tests_multithread/ -B build/debug-san-gcc/ -c test.txt

Содержимое test.txt:
vector
1 2 3 4
send
send
send
send
send
empty
quit

ДЛЯ РАБОТЫ СКРИПТА НЕОБХОДИМО ЗАПУСТИТЬ СЕРВЕР (proteip.server) И ПЕРЕДАТЬ ДИРЕКТОРИЮ С БИНАРНИКОМ. 
"

while getopts "a:p:n:t:N:B:c:" flag; do
        case "${flag}" in
                a) ADDRESS="$OPTARG";;
                p) PORT="$OPTARG";;
                n) CLIENTW="$OPTARG";;
                N) CLIENTN="$OPTARG";;
                B) BUILD_DIRECTORY="$OPTARG";;
                t) TEST_DIR="$OPTARG";;
                c) COMMAND_FILE="$OPTARG";;
                *) printf "$HELP_TEXT" $0 
                        exit 0;;
        esac
done

if [ ! -f "$COMMAND_FILE" ]; then
        echo "NO FILE WITH COMMANDS"
        exit 1
fi



if [ "${BUILD_DIRECTORY}" == "" ]; then
        printf "$HELP_TEXT" $0
        exit 1
fi

sleep .5
if [ ! -d "$TEST_DIR" ]; then
        mkdir -p "$TEST_DIR"
fi

for ((i = 0 ; i <= $CLIENTW ; i++))
do
        for ((j = 0 ; j <= $CLIENTN ; j++))  
        do
                CLIENT_LOG_N=$((i * 10 + j))
                cat ${COMMAND_FILE} | ./$BUILD_DIRECTORY/proteip -a $ADDRESS $PORT > "${TEST_DIR}output${CLIENT_LOG_N}.txt" &
        done
done

CONNECTIONS_N=$(( CLIENTN * CLIENTW / 2 + 1))

SLEEP_TIME=""

if [ $CONNECTIONS_N -ge 10 ]; then
        SLEEP_TIME+="$((CONNECTIONS_N / 10)).$((CONNECTIONS_N % 10))" 
else
        SLEEP_TIME+="0.${CONNECTIONS_N}"
fi

sleep "${SLEEP_TIME}"

DIFF_FILE="output0.txt"
RESULT_FILE="/test_result.txt"
IS_THERE_DIFF=false

for i in $(ls "$TEST_DIR");
do
        DIFFERENCE=$(diff -r "$TEST_DIR$i" "$TEST_DIR")
        if [ "$DIFFERENCE" ]; then
                echo $DIFFERENCE
                IS_THERE_DIFF=true
                exit 1
        fi
        
        if [ "$i" != "$DIFF_FILE" ]; then
                rm "${TEST_DIR}${i}"
        fi
done

mv "${TEST_DIR}${DIFF_FILE}" "${TEST_DIR}${RESULT_FILE}"
RESULT=$(cat $TEST_DIR${RESULT_FILE} | grep "Error") 
if [ "$RESULT" != "" ]; then
        echo "Error occured, check logs"
        exit 1
fi

echo "Test done correctly" 
