#include "commands.h"
#include "commands_internal.h"
#include <string.h>
#include <stdio.h>
#include "screenshot.h"

static int command_help(int argc, char *const *argv) {
    (void)argv;
    if (argc != 1) {
        fputs("사용법: help\n", stderr);
        return 2;
    }
    puts("========== Jiwonmin Shell 명령어 도움말 ==========");
    puts("[파일 및 디렉터리]");
    puts("ls [경로]          : 파일 및 디렉터리 목록 출력");
    puts("pwd                : 현재 작업 디렉터리 출력");
    puts("cd [경로]          : 작업 디렉터리 변경");
    puts("mkdir 경로         : 디렉터리 생성");
    puts("rmdir 경로         : 빈 디렉터리 삭제");
    puts("ln 원본 링크       : 링크 생성 (-s: 심볼릭 링크)");
    puts("cp 원본 대상       : 파일 복사");
    puts("rm 파일            : 파일 삭제");
    puts("mv 원본 대상       : 파일 이동");
    puts("cat [파일 ...]     : 파일 내용 출력");
    puts("grep 문자열 [파일] : 문자열 검색 (정규식 미지원)");
    puts("[쉘 제어]");
    puts("history [개수]     : 이전 명령어 목록 (최근 개수만 표시)");
    puts("history -c        : 명령어 기록 삭제");
    puts("!! / !번호        : 이전 명령어 재실행");
    puts("jobs               : 작업 목록 확인");
    puts("fg [작업번호]      : 작업을 포그라운드로 전환");
    puts("exit               : 쉘 종료");
    puts("help               : 이 도움말 출력");
    puts("[화면 캡처 - X11]");
    puts("jiwonmin           : 전체 화면 PNG 캡처");
    puts("jiwonmin -d 3      : 3초 후 캡처");
    puts("jiwonmin -o 파일   : 지정한 파일 경로에 저장");
    puts("[특수 기능]");
    puts("| : 파이프  < : 입력 재지향  > : 출력 재지향");
    puts(">> : 출력 추가  & : 백그라운드 실행");
    puts("Ctrl+C : 인터럽트  Ctrl+Z : 일시정지");
    puts("※ 일부 명령어는 기본 기능만 지원하며 옵션이 제한됩니다.");
    puts("=================================================");
    return 0;
}

int commands_execute(int argc, char *const *argv) {
    if (!argc) return -1;
    const char *cmd = argv[0];
    if (!strcmp(cmd, "help")) return command_help(argc, argv);
    if (!strcmp(cmd, "jiwonmin")) return screenshot_execute(argc, argv);
    if (!strcmp(cmd, "pwd")) return command_pwd(argc, argv);
    if (!strcmp(cmd, "ls")) return command_ls(argc, argv);
    if (!strcmp(cmd, "mkdir")) return command_mkdir(argc, argv);
    if (!strcmp(cmd, "rmdir")) return command_rmdir(argc, argv);
    if (!strcmp(cmd, "ln")) return command_ln(argc, argv);
    if (!strcmp(cmd, "cp")) return command_cp(argc, argv);
    if (!strcmp(cmd, "rm")) return command_rm(argc, argv);
    if (!strcmp(cmd, "mv")) return command_mv(argc, argv);
    if (!strcmp(cmd, "cat")) return command_cat(argc, argv);
    if (!strcmp(cmd, "grep")) return command_grep(argc, argv);
    return -1;
}
