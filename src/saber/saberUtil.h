#ifndef SABER_UTIL_INCLUDED
#define SABER_UTIL_INCLUDED

#include <stdint.h>
/* Don't include pins.h, etc. Keep this header (but not cpp)
   cross platform so that it can be included with test / debug
   code.
*/
//#include "pins.h" 

class Blade;
class SaberDB;

enum {
    BLADE_OFF,
    BLADE_IGNITE,
    BLADE_ON,
    BLADE_FLASH,        // private; set by the blade
    BLADE_RETRACT
};

class BladeState
{
public:
    BladeState() {}

    void change(uint8_t newState);
    const int state() const {
        return m_currentState;
    }

    // Any of the blade-on states, not just the BLADE_ON idles state.
    bool bladeOn() const;
    bool bladeOff() const {
        return m_currentState == BLADE_OFF;
    }

    const uint32_t startTime() const {
        return m_startTime;
    }

    void enableFlash(bool enable);
    bool flashEnabled() const { return m_flash; }

    void process(Blade* blade, const SaberDB& saberDB, uint32_t time);

private:
    uint32_t calcReflashTime() const;

    bool     m_flash = false;    
    uint8_t  m_currentState = BLADE_OFF;
    uint32_t m_startTime = 0;
    uint32_t m_reflashTime = 0;
};

enum class UIMode {
    NORMAL,
    PALETTE,
    VOLUME,
	MEDITATION
};

class UIModeUtil
{
public:
	UIModeUtil() {}

	void set(UIMode mode) { m_mode = mode; }
    void nextMode();
    
    const UIMode mode() const {
        return m_mode;
    }

private:
    UIMode m_mode = UIMode::NORMAL;
};

/**
 * Power changes over time, and higher
 * draw changes the power. A small class
 * to average out power changes.
 */
class AveragePower
{
public:
    AveragePower();
    void push(uint32_t milliVolts);
    uint32_t power() {
        return m_power;
    }
    enum { NUM_SAMPLES = 8};

private:
    uint32_t m_power;
    uint32_t m_sample[NUM_SAMPLES];
};

class Accelerometer
{
public:
    Accelerometer();
    static Accelerometer& instance() { return *_instance; }

    void begin();
    void read(float* ax, float* ay, float* az, float* g2, float* g2normal);

private:
    static Accelerometer* _instance;



};

class Voltmeter
{
public:
    Voltmeter() {}

    void begin();

    /// Instantaneous power. (Noisy).
    uint32_t readVBat();
    /// Average power.
    uint32_t averagePower() { return m_averagePower.power(); }

    /// Add a sample to the average power.
    void takeSample();

private:
    AveragePower m_averagePower;
};


#endif // SABER_UTIL_INCLUDED
