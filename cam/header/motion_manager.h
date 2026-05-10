#ifndef MOTION_MANAGER_H
#define MOTION_MANAGER_H

#include "buffer_manager.h"

// 움직임 감지 신호를 전달한다.
bool notify_motion_detected();

// motion 상태를 갱신한다.
// 완료된 segment가 있으면 motion event에 추가하고,
// 마지막 움직임 이후 설정 시간이 지나면 event를 확정한다.
bool update_motion_manager(const BufferUpdateResult& buffer_result);

#endif