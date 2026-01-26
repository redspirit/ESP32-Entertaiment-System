#pragma once

#include "PinConfig.h"
#include "Mode.h"
#include "DMAVideoBuffer.h"

class VGA
{
	public:
	Mode mode;
	int bufferCount;
	int bits;
	PinConfig pins;
	int backBuffer;
	DMAVideoBuffer *dmaBuffer;
	bool usePsram;
	int dmaChannel;
	
	public:
		VGA();
		~VGA();
		bool init(const PinConfig cfgPins, const Mode mode);
		bool start();
		bool show();
		void clear(uint8_t color);
		void fillRect(int x, int y, int w, int h, int rgb);
		void dot(int x, int y, uint8_t r, uint8_t g, uint8_t b);
		void dot(int x, int y, int rgb);

	protected:
		void attachPinToSignal(int pin, int signal);
};