#ifndef CAM_CONFIG_H
#define CAM_CONFIG_H

// 테스트 모드
// 1: 터미널에서 m 입력으로 가짜 움직임 발생
// 0: 실제 서비스 모드
#define CAM_TEST_MODE 1

// 더미 segment 길이
#define CAM_SEGMENT_SECONDS 5

// 움직임 감지 전 보관할 시간
#define CAM_PRE_MOTION_SECONDS 10

// 마지막 움직임 이후 추가 저장할 시간
#define CAM_POST_MOTION_SECONDS 10

// 더미 current segment에 기록할 주기
#define CAM_BUFFER_WRITE_INTERVAL_MS 1000

// main loop sleep 단위
#define CAM_MAIN_LOOP_SLEEP_MS 100

#endif