#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

// buffer manager를 초기화한다.
// 테스트용 current buffer 파일을 준비한다.
bool init_buffer_manager();

// current buffer를 prev buffer로 교체하고,
// 새로운 current buffer 파일을 생성한다.
bool rotate_buffer_file();

#endif