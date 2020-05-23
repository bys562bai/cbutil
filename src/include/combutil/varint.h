#pragma once
#include <stdint.h>
/*
BE 左高右低
LE 左低右高
fu8 首字节
BE 数据段靠右，长度段靠左
LE 数据段靠左，长度段靠右
*/
namespace combutil{

inline uint64_t read_uintBE(const uint8_t src[], uint8_t len){
    uint64_t ret = 0;
    for(int i=0; i<len; i++){
        ret<<=8;
        ret|=src[i];
    }
    return ret;
}

inline uint64_t read_uintLE(const uint8_t src[] ,uint8_t len){
    uint64_t ret = 0;
    for(int i=len-1; i>=0 ; i--){
        ret<<=8;
        ret|=src[i];
    }
    return ret;
}

inline uint8_t varintBE_suffix_len(uint8_t fu8){
    static const uint8_t highzero4_loc[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4}; 
    //[(bin(i)[2:].zfill(4)+'0').index('0') for i in range(16)]
    uint8_t h = highzero4_loc[fu8>>4];
    if (h<4) return h;
    else return 4 + highzero4_loc[fu8&0xF];
}

inline uint8_t varintLE_suffix_len(uint8_t fu8){
    static const uint8_t lowzero_loc[16] = {0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4}; 
    //[(bin(i)[2:].zfill(4)[-1::-1] +'0').index('0') for i in range(16)]
    uint8_t l = lowzero_loc[fu8&0xF];
    if (l<4) return l;
    else return 4 + lowzero_loc[fu8>>4];
}

inline uint8_t varintBE_len(uint8_t fu8){
    return varintBE_suffix_len(fu8) + 1;
}

inline uint8_t varintLE_len(uint8_t fu8){
    return varintLE_suffix_len(fu8) + 1;
}

inline uint8_t u16_len(uint16_t x){
    return 1 + ((bool) (x>>8));
}

inline uint8_t u32_len(uint32_t x){
    uint16_t t = x>>2*8;
    return t ? 2+u16_len(t) : u16_len(x);
}

inline uint8_t u64_len(uint64_t x){
    uint32_t t = x>>4*8;
    return t ? 4+u32_len(t) : u32_len(x);
}

inline uint64_t varintBE_to_u64(uint8_t pvarint[]){
    static const uint8_t masks[9] = {255, 127, 63, 31, 15, 7, 3, 1, 0}; //255 not used
    //[0xFF>>i for i in range(9)]
    uint8_t fu8 = pvarint[0];
    uint8_t len = varintBE_len(fu8);
    
    if (len == 9){
        return read_uintBE(pvarint+1, 8);
    }else{
        uint64_t ret = read_uintBE(pvarint, len);
        ret&= masks[len]<<7*8;
        return ret;
    }
}

inline uint64_t varintLE_to_u64(uint8_t pvarint[]){
    uint8_t fu8 = pvarint[0];
    uint8_t len = varintLE_len(fu8);
    if (len == 9){
        return read_uintLE(pvarint+1, 8);
    }else{
        uint64_t ret = read_uintLE(pvarint, len);
        ret >>= len;
        return ret;
    }
}

inline uint8_t write_varintBE(uint64_t value, uint8_t dst[]){
    static const uint8_t masks_table[9]= {0, 128, 192, 224, 240, 248, 252, 254, 255}; 
/*
l = []
m = 0
for i in range(8):
    m|= 1<<7-i
    l.append(m)
*/
    uint8_t len = u64_len(value);
    uint8_t mask = masks_table[len];
    len += mask & (value>>8*(len-1));

    const uint64_t write_mask = 0xFF;
    for(int i = len-1; i>=0; i--){
        dst[i] = value&write_mask;
        value>>=8;
    }
    dst[0]|=masks_table[len-1];
    return len;
}


inline uint8_t write_varintLE(uint64_t value, uint8_t dst[]){
    static const uint8_t masks_table[9]= {0, 128, 192, 224, 240, 248, 252, 254, 255}; 
    static const uint8_t low_masks_table[9] = {0, 1, 3, 7, 15, 31, 63, 127, 255};
/*
l = [0]
m = 0
for i in range(8):
    m|= 1<<i
    l.append(m)
*/
    uint8_t len = u64_len(value);
    uint8_t mask = masks_table[len];
    len += mask & (value>>8*(len-1));
    value<<=len;
    value|=low_masks_table[len];

    const uint64_t write_mask = 0xFF;
    for(int i = 0; i<len; i++){
        dst[i] = value&write_mask;
        value>>=8;
    }
    return len;
}

}