/*
  IonoModbusRtuSlave.cpp - Modbus RTU Slave library for Iono Arduino and Iono MKR

    Copyright (C) 2018 Sfera Labs S.r.l. - All rights reserved.

    For information, see the iono web site:
    http://www.sferalabs.cc/iono-arduino

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#include "IonoModbusRtuSlave.h"
#include <Iono.h>

#ifdef IONO_MKR
#define DO_MAX_INDEX 4
#else
#define DO_MAX_INDEX 6
#endif

bool IonoModbusRtuSlaveClass::_di1deb;
bool IonoModbusRtuSlaveClass::_di2deb;
bool IonoModbusRtuSlaveClass::_di3deb;
bool IonoModbusRtuSlaveClass::_di4deb;
bool IonoModbusRtuSlaveClass::_di5deb;
bool IonoModbusRtuSlaveClass::_di6deb;

word IonoModbusRtuSlaveClass::_di1count = 0;
word IonoModbusRtuSlaveClass::_di2count = 0;
word IonoModbusRtuSlaveClass::_di3count = 0;
word IonoModbusRtuSlaveClass::_di4count = 0;
word IonoModbusRtuSlaveClass::_di5count = 0;
word IonoModbusRtuSlaveClass::_di6count = 0;

void IonoModbusRtuSlaveClass::begin(byte unitAddr, unsigned long baud, unsigned long config, unsigned long diDebounceTime) {
  SERIAL_PORT_HARDWARE.begin(baud, config);
  ModbusRtuSlave.setCallback(&IonoModbusRtuSlaveClass::onRequest);
  ModbusRtuSlave.begin(unitAddr, &SERIAL_PORT_HARDWARE, baud, PIN_TXEN);

  Iono.subscribeDigital(DI1, diDebounceTime, &onDIChange);
  Iono.subscribeDigital(DI2, diDebounceTime, &onDIChange);
  Iono.subscribeDigital(DI3, diDebounceTime, &onDIChange);
  Iono.subscribeDigital(DI4, diDebounceTime, &onDIChange);
  Iono.subscribeDigital(DI5, diDebounceTime, &onDIChange);
  Iono.subscribeDigital(DI6, diDebounceTime, &onDIChange);
}

void IonoModbusRtuSlaveClass::process() {
  ModbusRtuSlave.process();
  Iono.process();
}

void IonoModbusRtuSlaveClass::onDIChange(uint8_t pin, float value) {
  switch (pin) {
    case DI1:
      _di1deb = value == HIGH;
      if (_di1deb) {
        _di1count++;
      }
      break;

    case DI2:
      _di2deb = value == HIGH;
      if (_di2deb) {
        _di2count++;
      }
      break;

    case DI3:
      _di3deb = value == HIGH;
      if (_di3deb) {
        _di3count++;
      }
      break;

    case DI4:
      _di4deb = value == HIGH;
      if (_di4deb) {
        _di4count++;
      }
      break;

    case DI5:
      _di5deb = value == HIGH;
      if (_di5deb) {
        _di5count++;
      }
      break;

    case DI6:
      _di6deb = value == HIGH;
      if (_di6deb) {
        _di6count++;
      }
      break;
  }
}

byte IonoModbusRtuSlaveClass::onRequest(byte unitAddr, byte function, word regAddr, word qty, byte *data) {
  switch (function) {
    case MB_FC_READ_COILS:
      if (checkAddrRange(regAddr, qty, 1, DO_MAX_INDEX)) {
        for (int i = regAddr; i < regAddr + qty; i++) {
          ModbusRtuSlave.responseAddBit(Iono.read(indexToDO(i)) == HIGH);
        }
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_READ_DISCRETE_INPUTS:
      if (checkAddrRange(regAddr, qty, 101, 106)) {
        for (int i = regAddr - 100; i < regAddr - 100 + qty; i++) {
          ModbusRtuSlave.responseAddBit(indexToDIdeb(i));
        }
        return MB_RESP_OK;
      }
      if (checkAddrRange(regAddr, qty, 111, 116)) {
        for (int i = regAddr - 110; i < regAddr - 110 + qty; i++) {
          ModbusRtuSlave.responseAddBit(Iono.read(indexToDI(i)) == HIGH);
        }
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_READ_HOLDING_REGISTERS:
      if (regAddr == 601 && qty == 1) {
        ModbusRtuSlave.responseAddRegister(Iono.read(AO1) * 1000);
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_READ_INPUT_REGISTER:
      if (checkAddrRange(regAddr, qty, 201, 204)) {
        for (int i = regAddr - 200; i < regAddr - 200 + qty; i++) {
          ModbusRtuSlave.responseAddRegister(Iono.read(indexToAV(i)) * 1000);
        }
        return MB_RESP_OK;
      }
      if (checkAddrRange(regAddr, qty, 301, 304)) {
        for (int i = regAddr - 300; i < regAddr - 300 + qty; i++) {
          ModbusRtuSlave.responseAddRegister(Iono.read(indexToAI(i)) * 1000);
        }
        return MB_RESP_OK;
      }
      if (checkAddrRange(regAddr, qty, 1001, 1006)) {
        for (int i = regAddr - 1000; i < regAddr - 1000 + qty; i++) {
          ModbusRtuSlave.responseAddRegister(indexToDIcount(i));
        }
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_WRITE_SINGLE_COIL:
      if (regAddr >= 1 && regAddr <= DO_MAX_INDEX) {
        bool on = ModbusRtuSlave.getDataCoil(function, data, 0);
        Iono.write(indexToDO(regAddr), on ? HIGH : LOW);
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_WRITE_SINGLE_REGISTER:
      if (regAddr == 601) {
        word value = ModbusRtuSlave.getDataRegister(function, data, 0);
        if (value < 0 || value > 10000) {
          return MB_EX_ILLEGAL_DATA_VALUE;
        }
        Iono.write(AO1, value / 1000.0);
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_WRITE_MULTIPLE_COILS:
      if (checkAddrRange(regAddr, qty, 1, DO_MAX_INDEX)) {
        for (int i = regAddr; i < regAddr + qty; i++) {
          bool on = ModbusRtuSlave.getDataCoil(function, data, i - regAddr);
          Iono.write(indexToDO(i), on ? HIGH : LOW);
        }
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    default:
      return MB_EX_ILLEGAL_FUNCTION;
  }
}

bool IonoModbusRtuSlaveClass::checkAddrRange(word regAddr, word qty, word min, word max) {
  return regAddr >= min && regAddr <= max && regAddr + qty <= max + 1;
}

uint8_t IonoModbusRtuSlaveClass::indexToDO(int i) {
  switch (i) {
    case 1:
      return DO1;
    case 2:
      return DO2;
    case 3:
      return DO3;
    case 4:
      return DO4;
    case 5:
      return DO5;
    case 6:
      return DO6;
  }
}

uint8_t IonoModbusRtuSlaveClass::indexToDI(int i) {
  switch (i) {
    case 1:
      return DI1;
    case 2:
      return DI2;
    case 3:
      return DI3;
    case 4:
      return DI4;
    case 5:
      return DI5;
    case 6:
      return DI6;
  }
}

bool IonoModbusRtuSlaveClass::indexToDIdeb(int i) {
  switch (i) {
    case 1:
      return _di1deb;
    case 2:
      return _di2deb;
    case 3:
      return _di3deb;
    case 4:
      return _di4deb;
    case 5:
      return _di5deb;
    case 6:
      return _di6deb;
  }
}

word IonoModbusRtuSlaveClass::indexToDIcount(int i) {
  switch (i) {
    case 1:
      return _di1count;
    case 2:
      return _di2count;
    case 3:
      return _di3count;
    case 4:
      return _di4count;
    case 5:
      return _di5count;
    case 6:
      return _di6count;
  }
}

uint8_t IonoModbusRtuSlaveClass::indexToAV(int i) {
  switch (i) {
    case 1:
      return AV1;
    case 2:
      return AV2;
    case 3:
      return AV3;
    case 4:
      return AV4;
  }
}

uint8_t IonoModbusRtuSlaveClass::indexToAI(int i) {
  switch (i) {
    case 1:
      return AI1;
    case 2:
      return AI2;
    case 3:
      return AI3;
    case 4:
      return AI4;
  }
}
