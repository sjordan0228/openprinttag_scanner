#ifndef HARDWARE_NFC_CONNECTION_H
#define HARDWARE_NFC_CONNECTION_H

#include "NFCConnectionI.h"
#include <PN5180.h>
#include <PN5180ISO15693.h>

// ---------------------------------------------------------------------------
// PN5180 board pin configuration
//
// These values may be supplied by PlatformIO build flags for different
// hardware profiles. If not provided, the defaults below preserve the
// existing ESP32 DevKit wiring.
// ---------------------------------------------------------------------------

#ifndef NFC_PIN_PN5180_RST
#define NFC_PIN_PN5180_RST 13
#endif

#ifndef NFC_PIN_PN5180_NSS
#define NFC_PIN_PN5180_NSS 14
#endif

#ifndef NFC_PIN_PN5180_MOSI
#define NFC_PIN_PN5180_MOSI 27
#endif

#ifndef NFC_PIN_PN5180_MISO
#define NFC_PIN_PN5180_MISO 26
#endif

#ifndef NFC_PIN_PN5180_SCK
#define NFC_PIN_PN5180_SCK 25
#endif

#ifndef NFC_PIN_PN5180_BUSY
#define NFC_PIN_PN5180_BUSY 33
#endif

#ifndef NFC_PIN_PN5180_GPIO
#define NFC_PIN_PN5180_GPIO 32
#endif

#ifndef NFC_PIN_PN5180_IRQ
#define NFC_PIN_PN5180_IRQ 35
#endif

#ifndef NFC_PIN_PN5180_AUX
#define NFC_PIN_PN5180_AUX 34
#endif

// Production NFC connection using PN5180 hardware
class HardwareNFCConnection : public NFCConnectionI {
public:
    HardwareNFCConnection();
    ~HardwareNFCConnection() override;

    bool begin() override;
    void reset() override;
    bool hardwareReset() override;
    bool setupRF() override;
    bool detectTag(uint8_t* uid, uint8_t* uidLength) override;
    void setCurrentUid(const uint8_t* uid, uint8_t length) override;
    opt_nfc_hal_t* getHal() override;

    // Diagnostics: log RF_STATUS, IRQ_STATUS, SYSTEM_STATUS registers
    void logDiagnostics();

private:
    PN5180ISO15693* nfc_ = nullptr;
    opt_nfc_hal_t hal_;
    uint8_t currentUid_[8];
    
    // PN5180 pin assignments are provided by build flags or fall back to
    // the defaults defined above. REQ (DWL_REQ) remains unconnected and is
    // only needed for PN5180 firmware updates.

    static constexpr int PN5180_RST  = NFC_PIN_PN5180_RST;
    static constexpr int PN5180_NSS  = NFC_PIN_PN5180_NSS;
    static constexpr int PN5180_MOSI = NFC_PIN_PN5180_MOSI;
    static constexpr int PN5180_MISO = NFC_PIN_PN5180_MISO;
    static constexpr int PN5180_SCK  = NFC_PIN_PN5180_SCK;
    static constexpr int PN5180_BUSY = NFC_PIN_PN5180_BUSY;
    static constexpr int PN5180_GPIO = NFC_PIN_PN5180_GPIO;
    static constexpr int PN5180_IRQ  = NFC_PIN_PN5180_IRQ;
    static constexpr int PN5180_AUX  = NFC_PIN_PN5180_AUX;
    
    // Static HAL callbacks
    static opt_error_t halReadPage(void* ctx, uint8_t page, uint8_t* buffer);
    static opt_error_t halWritePage(void* ctx, uint8_t page, const uint8_t* data);
};

#endif // HARDWARE_NFC_CONNECTION_H
