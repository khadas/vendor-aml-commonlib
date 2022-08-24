#ifndef EVENTS_PROCESS_H
#define EVENTS_PROCESS_H

#include <linux/input.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <sys/time.h>
#include <vector>

class EventsProcess {
  public:
    struct KeyMapItem_t {
        const char* type;
        int value;
        int mode;
    };
    EventsProcess();
    virtual ~EventsProcess() { }
    virtual void Init();
    virtual void WaitKey();
//    virtual void KeyLongPress(int key);

protected:
    void EnqueueKey(int key_code, bool flag);
private:
    pthread_mutex_t key_queue_mutex;
    pthread_cond_t key_queue_cond;
    int key_queue[256], key_queue_len;
    struct timeval last_queue_time;
    char key_pressed[KEY_MAX + 1];
    int key_last_down=0;
    int key_down_count=0;
    bool enable_reboot=0;
    int rel_sum=0;

    struct KeyEvent {
      int64_t down_ts_us;
      int64_t up_ts_us;
      int keyCode;
      int keyState;
      bool longPress;
      bool curlongPress;
    };
    std::vector<KeyEvent *> mKeyEventVec;

    int last_key;
    bool report_longpress_flag;
    struct key_timer_t {
        EventsProcess* ep;
        int key_code;
        int count;
        KeyEvent *ke;
    };

    int num_keys;
    KeyMapItem_t* keys_map;

    #define DEFAULT_KEY_NUM 3

    #define NUM_CTRLINFO 3

    pthread_t input_thread_ = 0;

    static int InputCallback(int fd, uint32_t epevents, void* data);
    int OnInputEvent(int fd, uint32_t epevents);
    void ProcessKey(int key_code, int updown);

    static void* time_key_helper(void* cookie);
//    void time_key(int key_code, int count);
    void time_key(key_timer_t *info);
    const char* getKeyType(int key);
    int getKeyMode(int key);
    void load_key_map();
    int getMapKey(int key);
    uint64_t getSystemTimeUs();
};

#endif  // RECOVERY_UI_H
