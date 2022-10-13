#include "gpio.h"
#include "gpiocmd.h"
#include "mpscq.hpp"

#include <cstdint>
#include <cctype>
#include <memory>
#include <vector>
#include <set>
#include <string_view>
#include <algorithm>
#include <optional>

namespace Gpio {


namespace {
bool IsDelimeter(uint8_t sym)
{
    switch (sym) {
    case '\n': case '\0': case '\r':
        return true;
    default:
        return false;
    };
}
};

int Commander::AppendData(const uint8_t *data, uint32_t len)
{
    return m_queue.Enque(data, data + len) ? 0 : -1;
}

int Commander::ProcessData()
{
    int rc = 0;

    auto data = m_queue.PopWait();
    DoResponse(data.data(), data.size());
    for (auto sym : data)
    {
        if (sym == 8) {
            m_store.pop_back();
            DoResponse(" \x8");
        } else if (IsDelimeter(sym) || !std::isprint(sym)) {
            DoResponse("\r\n");
            rc = DoRun();
            m_store = {};
        } else {
            m_store.emplace_back(tolower(sym));
        }
    }
    DoFlush();
    return rc;
}

int Commander::SetResponse(ResponserT rsp)
{
    m_responser = rsp;
    return 0;
}

Commander &Commander::Self()
{
    static Commander self {};

    return self;
}

int Commander::DoRun()
{
    Argv request;
    auto isdelim = [](auto c) { return !std::isprint(c) || std::isspace(c); };

    for (auto b = m_store.begin(), s = b; b != m_store.end();)
    {
        while (b != m_store.end() && isdelim(*b)) {
            ++b;
        }
        s = std::find_if(b, m_store.end(), isdelim);
        if (s != b) {
            request.emplace_back(b, s);
        }
        while (s != m_store.end() && isdelim(*b)) {
            ++s;
        }
        b = s;
    }
    DoCommand(request);
    return 0;
}

int Commander::DoFlush()
{
    int rc = 0;
    if (m_responser) {
        rc = m_responser(m_output.data(), m_output.size());
    }
    m_output.resize(0);
    return rc;
}

namespace {
auto Widthdrawer(const Commander::Argv &args)
{
    return [&args](size_t i) {
        std::optional<std::string_view> res {};
        if (args.size() > i) {
            res = args[i];
        }
        return res;
    };
}
}
#define GpioAssert(op, msg) do { if (!(op)) { DoResponse(msg "\r\n"); return -1; } } while (0)

int Commander::DoCommand(const Argv &args)
{
    auto widthdraw = Widthdrawer(args);

    auto cmd = widthdraw(0);
    GpioAssert(cmd, "No command specified");

    #define CASE(n)            \
    if (*cmd == #n) {          \
        return Do_ ## n(args); \
    } else

    try {

        CASE(up)
        CASE(down)
        CASE(toggle)
        CASE(get)
        CASE(init)
        CASE(deinit)
        GpioAssert(false, "Invalid command specified");
    } catch (...) {
        GpioAssert(false, "Exception within parse command");
    }
}

template <typename PinT>
int Commander::ReadPin(const Argv &args, GPIO_TypeDef *&out_reg, PinT &out_pin, int argn)
{
    auto widthdraw = Widthdrawer(args);
    auto reg = widthdraw(argn+0);
    auto pin = widthdraw(argn+1);

    GpioAssert(reg, "No register specified");
    GpioAssert(reg->size() == 1, "Invalid register specified");

    switch ((*reg)[0]) {
    case 'a': out_reg = GPIOA; break;
    case 'b': out_reg = GPIOB; break;
    case 'c': out_reg = GPIOC; break;
    case 'd': out_reg = GPIOD; break;
    case 'e': out_reg = GPIOE; break;
    };

    GpioAssert(out_reg, "Invalid register name");

    if(!pin) {
        out_pin = GPIO_PIN_All;
        return 0;
    }

    bool pinMaskMode = (*pin)[0] == 'm';

    if (pinMaskMode) {
        pin = {pin->begin() + 1, pin->end()};
    }

    const std::string spin = {pin->begin(), pin->end()};
    unsigned long n = std::stoul(spin, nullptr, 0);
    if (!pinMaskMode) {
        unsigned long mask = 0x1ul << n;
        GpioAssert(mask != 0 || n == 0, "Invalid pin number");
        n = mask;
    }
    GpioAssert((n & GPIO_PIN_MASK) == n, "Invalid pin mask");
    out_pin = n;
    return 0;
}

int Commander::Do_up(const Argv &args)
{
    uint16_t pin = GPIO_PIN_All;
    GPIO_TypeDef *reg = nullptr;

    if (ReadPin(args, reg, pin)) {
        return -1;
    }

    HAL_GPIO_WritePin(reg, pin, GPIO_PIN_SET);
    return 0;
}

int Commander::Do_down(const Argv &args)
{
    uint16_t pin = GPIO_PIN_All;
    GPIO_TypeDef *reg = nullptr;

    if (ReadPin(args, reg, pin)) {
        return -1;
    }

    HAL_GPIO_WritePin(reg, pin, GPIO_PIN_RESET);
    return 0;
}

int Commander::Do_toggle(const Argv &args)
{
    uint16_t pin = GPIO_PIN_All;
    GPIO_TypeDef *reg = nullptr;

    if (ReadPin(args, reg, pin)) {
        return -1;
    }

    HAL_GPIO_TogglePin(reg, pin);
    return 0;
}

int Commander::Do_get(const Argv &args)
{
    uint16_t pin = GPIO_PIN_All;
    GPIO_TypeDef *reg = nullptr;

    if (ReadPin(args, reg, pin)) {
        return -1;
    }

    switch(HAL_GPIO_ReadPin(reg, pin))
    {
    case GPIO_PIN_SET:
        DoResponse("up\r\n");
        break;
    case GPIO_PIN_RESET:
        DoResponse("down\r\n");
        break;
    default:
        DoResponse("invalid\r\n");
        break;
    }
    return 0;
}

int Commander::Do_init(const Argv &args)
{
    auto widthdraw = Widthdrawer(args);
    GPIO_TypeDef *reg = nullptr;
    GPIO_InitTypeDef init = {
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_MEDIUM,
    };
    auto mode = widthdraw(3);

    if (ReadPin(args, reg, init.Pin)) {
        return -1;
    }

    GpioAssert(mode, "Mode required");
    if (mode == "in") {
        init.Mode = GPIO_MODE_INPUT;
    } else if (mode == "out") {
        init.Mode = GPIO_MODE_OUTPUT_OD;
    } else {
        GpioAssert(false, "Invalid mode");
    }
    HAL_GPIO_Init(reg, &init);
    return 0;
}

int Commander::Do_deinit(const Argv &args)
{
    uint16_t pin = GPIO_PIN_All;
    GPIO_TypeDef *reg = nullptr;

    if (ReadPin(args, reg, pin)) {
        return -1;
    }

    HAL_GPIO_DeInit(reg, pin);
    return 0;
}

int Commander::DoResponse(std::string_view data)
{
    return DoResponse(reinterpret_cast<const uint8_t *>(data.data()), data.size());
}

int Commander::DoResponse(const uint8_t *data, size_t len)
{
    m_output.reserve(m_output.size() + len);

    for (;len > 0; ++data, --len)
    {
        m_output.emplace_back(*data);
    }
    return 0;
}

};


extern "C" int GpioCmd_AppendData(const uint8_t *data, uint32_t len)
{
    return Gpio::Commander::Self().AppendData(data, len);
}


extern "C" int GpioCmd_SetResponse(ResponserT rsp)
{
    return Gpio::Commander::Self().SetResponse(rsp);
}

extern "C" int GpioCmd_Process()
{
    return Gpio::Commander::Self().ProcessData();
}
