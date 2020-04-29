#include "mbed.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#define UINT14_MAX 16383
// FXOS8700CQ I2C address
#define FXOS8700CQ_SLAVE_ADDR0 (0x1E << 1) // with pins SA0=0, SA1=0
#define FXOS8700CQ_SLAVE_ADDR1 (0x1D << 1) // with pins SA0=1, SA1=0
#define FXOS8700CQ_SLAVE_ADDR2 (0x1C << 1) // with pins SA0=0, SA1=1
#define FXOS8700CQ_SLAVE_ADDR3 (0x1F << 1) // with pins SA0=1, SA1=1
// FXOS8700CQ internal register addresses
#define FXOS8700Q_STATUS 0x00
#define FXOS8700Q_OUT_X_MSB 0x01
#define FXOS8700Q_OUT_Y_MSB 0x03
#define FXOS8700Q_OUT_Z_MSB 0x05
#define FXOS8700Q_M_OUT_X_MSB 0x33
#define FXOS8700Q_M_OUT_Y_MSB 0x35
#define FXOS8700Q_M_OUT_Z_MSB 0x37
#define FXOS8700Q_WHOAMI 0x0D
#define FXOS8700Q_XYZ_DATA_CFG 0x0E
#define FXOS8700Q_CTRL_REG1 0x2A
#define FXOS8700Q_M_CTRL_REG1 0x5B
#define FXOS8700Q_M_CTRL_REG2 0x5C
#define FXOS8700Q_WHOAMI_VAL 0xC7

I2C i2c(PTD9, PTD8);
Serial pc(USBTX, USBRX);
int m_addr = FXOS8700CQ_SLAVE_ADDR1;
uint8_t sdata[2], res[6];
int16_t acc16;
float t[3];

DigitalOut redLED(LED1);
InterruptIn trig(SW2);
EventQueue queue;
float buffer[3][101];
int tiltbuf[101];
int bi = 0;

void FXOS8700CQ_readRegs(int addr, uint8_t *data, int len);
void FXOS8700CQ_writeRegs(uint8_t *data, int len);
void logAccel();
void readAccel();

int main()
{
	pc.baud(115200);
	redLED = 1;

	// Enable the FXOS8700Q
	FXOS8700CQ_readRegs(FXOS8700Q_CTRL_REG1, &sdata[1], 1);
	sdata[1] |= 0x01;
	sdata[0] = FXOS8700Q_CTRL_REG1;
	FXOS8700CQ_writeRegs(sdata, 2);

	// Setup EventQueue
	queue.call_every(100, logAccel);

	while (true)
	{
		if (!trig)
		{
			bi = 0;

			queue.dispatch(9999);

			redLED = 1;
			for (int i = 0; i < 101; i++)
			{
				pc.printf("%1.4f,%1.4f,%1.4f,%d\r\n", buffer[0][i], buffer[1][i], buffer[2][i], tiltbuf[i]);
			}
		}

		wait(0.05);
	}
}

void readAccel()
{
	FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);

	acc16 = (res[0] << 6) | (res[1] >> 2);
	if (acc16 > UINT14_MAX / 2)
		acc16 -= UINT14_MAX;
	t[0] = ((float)acc16) / 4096.0f;

	acc16 = (res[2] << 6) | (res[3] >> 2);
	if (acc16 > UINT14_MAX / 2)
		acc16 -= UINT14_MAX;
	t[1] = ((float)acc16) / 4096.0f;

	acc16 = (res[4] << 6) | (res[5] >> 2);
	if (acc16 > UINT14_MAX / 2)
		acc16 -= UINT14_MAX;
	t[2] = ((float)acc16) / 4096.0f;
}

void logAccel()
{
	redLED = !redLED;
	readAccel();

	for (int i = 0; i < 3; i++)
		buffer[i][bi] = t[i];

	if (abs(t[0]) > 0.5 || abs(t[1]) > 0.5)
	{
		tiltbuf[bi] = 1;
	}
	else
	{
		tiltbuf[bi] = 0;
	}

	bi++;
}

void FXOS8700CQ_readRegs(int addr, uint8_t *data, int len)
{
	char t = addr;
	i2c.write(m_addr, &t, 1, true);
	i2c.read(m_addr, (char *)data, len);
}

void FXOS8700CQ_writeRegs(uint8_t *data, int len)
{
	i2c.write(m_addr, (char *)data, len);
}