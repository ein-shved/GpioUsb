#ifndef __GPIOCMD_H
#define __GPIOCMD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef int (*ResponserT)(const uint8_t *data, uint16_t len);
int GpioCmd_SetResponse(ResponserT rsp);
int GpioCmd_AppendData(const uint8_t *data, uint32_t len);
int GpioCmd_Process();

#ifdef __cplusplus
}

#include <vector>
#include <memory>
#include "mpscq.hpp"

namespace Gpio {
class Commander {
public:
    using Argv = std::vector<std::string_view>;
    int ProcessData(void);
    int AppendData(const uint8_t *data, uint32_t len);
    int SetResponse(ResponserT rsp);
    static Commander &Self();
private:
    using DataStore = std::vector<char>;
private:
    int DoRun();
    int DoResponse();
    int DoResponse(std::string_view data);
    int DoResponse(const uint8_t *data, size_t len);
    int DoFlush();
    int DoCommand(const Argv &args);

    int Do_up(const Argv &args);
    int Do_down(const Argv &args);
    int Do_toggle(const Argv &args);
    int Do_get(const Argv &args);
    int Do_init(const Argv &args);
    int Do_deinit(const Argv &args);

    template <typename PinT>
    int ReadPin(const Argv &args, GPIO_TypeDef *&out_reg, PinT &out_pin, int argn=1);
private:
    DataStore m_store {};
    ResponserT m_responser = nullptr;

    MPSC::Queue<std::vector<uint8_t>> m_queue {};
    std::vector<uint8_t> m_output;
};
}

#endif

#endif /* __GPIOCMD_H */
