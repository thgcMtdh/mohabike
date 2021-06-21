#include <stdio.h>

void set_mem(void* dest, void* src, size_t size) {
    for (int i=0; i<size; i++) {
        *((uint8_t*)dest + i) = *((uint8_t*)src + i);
    }
}

int main() {

    /* 浮動小数点数のバイナリ表現をprintするには、
       一度当該変数のアドレスをint型ポインタにキャストし、
       その値を読む
    */
    float f1 = 0.11;
    printf("%f: %#x\n", f1, *((uint32_t*)&f1) );
    
    uint8_t data[10];
    for (size_t i=0; i<sizeof(data); i++) data[i] = 0;  // 初期化

    /* 値をdataの特定位置に書き込む */
    size_t pos = 1;  // 書き込みたい位置
    set_mem(data + pos, &f1, sizeof(f1));
    printf("%#x\n", data[1]);

    /* dataの特定位置にある値を読む */
    float* f2;
    f2 = (float*)(data + pos);
    printf("%f\n", *f2);
}