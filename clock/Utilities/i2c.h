#define I2C_READ    1
#define I2C_WRITE   0

void i2c_init(void);
void i2c_stop(void);
unsigned char i2c_start(unsigned char addr);
unsigned char i2c_rep_start(unsigned char addr);
void i2c_start_wait(unsigned char addr);
unsigned char i2c_write(unsigned char data);
unsigned char i2c_readAck(void);
unsigned char i2c_readNak(void);
unsigned char i2c_read(unsigned char ack);

#define i2c_read(ack)  (ack) ? i2c_readAck() : i2c_readNak(); 