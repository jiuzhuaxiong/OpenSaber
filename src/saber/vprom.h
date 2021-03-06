#ifndef VPROM_INCLUDED
#define VPROM_INCLUDED

#include <stdint.h>
#include <Arduino.h>

/* Virtual EEPROM support. */

#ifdef CORE_TEENSY
#include <EEPROM.h>

template<class T>
void vpromPut(uint32_t addr, const T& t) { EEPROM.put(addr, t); }
template <class T>
void vpromGet(uint32_t addr, T& t) { EEPROM.get(addr, t); }

#else
void vpromPutRaw(uint32_t addr, size_t size, const void* data);
void vpromGetRaw(uint32_t addr, size_t size, void* data);

template<class T>
void vpromPut(uint32_t addr, const T& t) { vpromPutRaw(addr, sizeof(T), &t); }
template <class T>
void vpromGet(uint32_t addr, T& t) { vpromGetRaw(addr, sizeof(T), &t); }
#endif

#endif // VPROM_INCLUDED