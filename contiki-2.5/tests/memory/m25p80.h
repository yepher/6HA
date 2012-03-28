
#define M25P80_CS_OUTPUT() DDRE|=(1<<3)
#define M25P80_CS_H() PORTE|=(1<<3)
#define M25P80_CS_L() PORTE&=(~(1<<3))

#define m25p80_WIP_read() (m25p80_rdsr() & 0x01)
#define m25p80_wait_end_write() while(m25p80_WIP_read())

void m25p80_init_cs(void);
void m25p80_wren(void);
void m25p80_wrdi(void);
void m25p80_dp(void);
void m25p80_be(void);
uint8_t m25p80_rdsr(void);
void m25p80_wrsr(uint8_t data);
uint8_t m25p80_res(void);
uint8_t m25p80_pp(uint32_t addr, uint16_t bytes, uint8_t * pdata);
uint8_t m25p80_read(uint32_t addr, uint16_t bytes, uint8_t * pdata);
uint8_t m25p80_se(uint32_t addr);

