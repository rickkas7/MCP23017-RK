#include "Particle.h"
#include "MCP23017-RK.h"


MCP23017::MCP23017(TwoWire &wire, int addr) :
	wire(wire), addr(addr) {
}

MCP23017::~MCP23017() {
}

void MCP23017::begin() {
	wire.begin();
}

void MCP23017::pinMode(uint16_t pin, PinMode mode) {
	if (!pinAvailable(pin)) {
		return;
	}

	// Set input or output mode
	// true for INPUT or INPUT_PULLUP or false for OUTPUT
	writeRegisterPin(REG_IODIR, pin, (mode != OUTPUT));

	// Set pull-ups (100K)
	if (mode == INPUT || mode == INPUT_PULLUP) {
		// true for INPUT_PULLUP, false for INPUT
		writeRegisterPin(REG_GPPU, pin, (mode == INPUT_PULLUP));
	}
}

PinMode MCP23017::getPinMode(uint16_t pin) {
	if (!pinAvailable(pin)) {
		return INPUT;
	}

	if (readRegisterPin(REG_IODIR, pin)) {
		// bit is 1 (INPUT)
		if (readRegisterPin(REG_GPPU, pin)) {
			// bit is 1
			return INPUT_PULLUP;
		}
		else {
			return INPUT;
		}
	}
	else {
		// bit is 0
		return OUTPUT;
	}
}

bool MCP23017::pinAvailable(uint16_t pin) {
	return pin < NUM_PINS;
}

void MCP23017::digitalWrite(uint16_t pin, uint8_t value) {
	if (!pinAvailable(pin)) {
		return;
	}
	writeRegisterPin(REG_OLAT, pin, (bool)value);
}

int32_t MCP23017::digitalRead(uint16_t pin) {
	if (!pinAvailable(pin)) {
		return HIGH;
	}

	if (readRegisterPin(REG_GPIO, pin)) {
		return HIGH;
	}
	else {
		return LOW;
	}
}

bool MCP23017::readRegisterPin(uint8_t reg, uint16_t pin) {
	if (!pinAvailable(pin)) {
		return false;
	}
	// Convert pin 0 - 15 to two banks of 0 - 7, with different registers for bank B (8-15)
	reg = mapRegister(reg, pin);
	pin = mapPin(pin);

	return readRegister(reg) & (1 << pin);
}

bool MCP23017::writeRegisterPin(uint8_t reg, uint16_t pin, bool value) {
	if (!pinAvailable(pin)) {
		return false;
	}

	// Convert pin 0 - 15 to two banks of 0 - 7, with different registers for bank B (8-15)
	reg = mapRegister(reg, pin);
	pin = mapPin(pin);

	uint8_t regValue = readRegister(reg);

	if (value) {
		regValue |= (1 << pin);
	}
	else {
		regValue &= ~(1 << pin);
	}

	return writeRegister(reg, regValue);
}


uint8_t MCP23017::readRegister(uint8_t reg) {
	wire.beginTransmission(addr | DEVICE_ADDR);
	wire.write(reg);
	wire.endTransmission(false);

	wire.requestFrom(addr | DEVICE_ADDR, 1, true);
	uint8_t value = (uint8_t) wire.read();

	// Serial.printlnf("readRegister reg=%d value=%d", reg, value);

	return value;
}

bool MCP23017::writeRegister(uint8_t reg, uint8_t value) {
	wire.beginTransmission(addr | DEVICE_ADDR);
	wire.write(reg);
	wire.write(value);

	int stat = wire.endTransmission(true);

	// Serial.printlnf("writeRegister reg=%d value=%d stat=%d read=%d", reg, value, stat, readRegister(reg));
	return (stat == 0);
}

uint8_t MCP23017::mapRegister(uint8_t reg, uint16_t pin) {
	// When IOCON == 0 (default), then registers are interleaved A, B, A, B
	if (pin >= 8) {
		reg++;
	}
	return reg;
}

uint16_t MCP23017::mapPin(uint16_t pin) {
	return (pin & 0x7);
}


