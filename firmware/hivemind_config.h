#define scene_name "";
#define cabinet_name "";
#define token "";
#define CAB_IP "kq.local"

//Define the pins for SPI communication.
#define PN532_SCK  (18)
#define PN532_MOSI (23)
#define PN532_SS   (14)
#define PN532_MISO (19)

//Button configs
#define skulls_button (32)
#define abs_button (39) // this one is VN and might be referenced wrong
#define checks_button (36) // This one is VP and might be referenced wrong
#define queen_button (35)
#define stripes_button (33)

//Light configs
int brightness = 125;
#define stripes_led (34)
#define queen_led (25)
#define checks_led (27)
#define abs_led (2)
#define skulls_led (15) // this one might be referenced wrong