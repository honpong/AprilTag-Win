/*
* INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement
* or nondisclosure agreement with Intel Corporation and may not be
* copied or disclosed except in accordance with the terms of that
* agreement.
* Copyright(c) 2014-2018 Intel Corporation. All Rights Reserved.
*/

#include "librealsense2/rs.hpp"
#include "librealsense2/rs_advanced_mode.hpp"

#include "rs2-custom-calibration-mm.h"

#include <cstring>
#include <cmath>
#include <vector>


using namespace rs2;
using namespace std;

#if defined(_WIN32)
#define DS_MEMCPY(a, c, d) memcpy_s(a, d, c, d)
#else

extern "C" {
//#include "safe_mem_lib.h"
}

#define DS_MEMCPY(a, c, d) memcpy(a, c, d)

#endif


// start of definitions for motion module custom data

#define DC_MM_EEPROM_SIZE             520   // 0x208
#define DC_MM_CAL_TABLE_OFFSET        96    // 0x60 = (0x60)                 --> calibration table
#define DC_MM_TM1_MODEL_OFFSET        112   // 0x70 = (0x60 + 0x10)          --> TM1 calibration model
#define DC_MM_FE_TABLE_OFFSET         132   // 0x84 = (0x60 + 0x10 + 0x14)   --> FE calibration table
#define DC_MM_FE_TABLE_SIZE           152   // 0x98
#define DC_MM_IMU_TABLE_OFFSET        308   // 0x84 = (0x60 + 0xd4)          --> IMU calibration table
#define DC_MM_IMU_TABLE_SIZE          180   // 0xB4

// motion module custom data including header
struct MMEEPROM
{
    uint8_t data[DC_MM_EEPROM_SIZE];
};

// motion module custom data header
struct MMTableHeader
{
    uint16_t version;                       //!< Table version
    uint16_t tableType;                     //!< Table type
    uint32_t tableSize;                     //!< Table size
    uint32_t resrved;                       //!< reserved field
    uint32_t crc32;                         //!< CRC32 of the content
};

// end of definitions for motion module custom data


// start of definitions for crc calculation
static uint32_t Crc32Table[] =
{
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t CalculateCrc32(uint8_t *buffer, int length);

uint32_t CalculateCrc32(uint8_t *buffer, int length)
{
	uint32_t crc32 = 0xFFFFFFFF;

	for (int i = 0; length > 0; --length, i++)
		crc32 = Crc32Table[((crc32) ^ (buffer[i])) & 0xff] ^ ((crc32) >> 8);

	return ~crc32;
}

// end of definitions for crc calculation


namespace crw
{
    #define DS5_CMD_LENGTH          24
    #define DS5_OUTPUT_BUFFER_SIZE  1024
    #define DS5_CMD_OPCODE_SIZE     4

    uint8_t GetFisheyeCalibrationData_Cmd[DS5_CMD_LENGTH] = { 0x14, 0x0, 0xab, 0xcd, 0x15, 0, 0, 0, 0x21, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint8_t SetFisheyeCalibrationData_Cmd[DS5_CMD_LENGTH] = { 0x14, 0x0, 0xab, 0xcd, 0x62, 0, 0, 0, 0x21, 0, 0, 0, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    uint8_t MMER_Cmd[DS5_CMD_LENGTH] = { 0x14, 0x0, 0xab, 0xcd, 0x4F, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint8_t MMEW_Cmd[DS5_CMD_LENGTH] = { 0x14, 0x0, 0xab, 0xcd, 0x50, 0, 0, 0, 0, 0, 0, 0, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


    MMCalibrationRWAPI::MMCalibrationRWAPI()
    {
        m_Rs400Device = nullptr;
    }

    MMCalibrationRWAPI::~MMCalibrationRWAPI()
    {
    }

    int MMCalibrationRWAPI::Initialize(void *rs400Dev)
    {
        if (rs400Dev == nullptr)
            return DC_MM_ERROR_INVALID_PARAMETER;

        m_Rs400Device = rs400Dev;

        return DC_MM_SUCCESS;
    }

    const char * MMCalibrationRWAPI::GetVersion()
    {
        return DS_CUSTOM_CALIBRATION_VERSION;
    }

    bool MMCalibrationRWAPI::HwMonitorCmd_Get(void* rs400Dev, uint8_t* cmd, uint8_t* data, int length)
    {
        try {
                device device = *(rs2::device *)rs400Dev;
                std::vector<uint8_t> RawBuffer(DS5_CMD_LENGTH);
                std::vector<uint8_t> rcvBuf;

                auto debug = device.as<debug_protocol>();
                DS_MEMCPY(RawBuffer.data(), cmd, DS5_CMD_LENGTH);
                rcvBuf = debug.send_and_receive_raw_data(RawBuffer);
                if (rcvBuf[0] != cmd[4])
                    return false;

                int len = (length <= (int)rcvBuf.size() - DS5_CMD_OPCODE_SIZE) ? length : (int)rcvBuf.size() - DS5_CMD_OPCODE_SIZE;

                DS_MEMCPY(data, rcvBuf.data() + DS5_CMD_OPCODE_SIZE, len);
            }
            catch (exception e)
            {
                return false;
            }

        return true;
    }

    bool MMCalibrationRWAPI::HwMonitorCmd_Set(void* rs400Dev, uint8_t* cmd, uint8_t* data, int length)
    {
        try {
                device device = *(rs2::device *)rs400Dev;
                std::vector<uint8_t> RawBuffer(DS5_CMD_LENGTH + length);
                std::vector<uint8_t> rcvBuf;
                DS_MEMCPY(RawBuffer.data(), cmd, DS5_CMD_LENGTH);
                if (length > 0)
                    DS_MEMCPY(RawBuffer.data() + DS5_CMD_LENGTH, data, length);
                uint16_t *snd_buf = (uint16_t *)RawBuffer.data();
                snd_buf[0] += length;

                auto debug = device.as<debug_protocol>();
                rcvBuf = debug.send_and_receive_raw_data(RawBuffer);
                if (rcvBuf[0] != cmd[4])
                    return false;
            }
            catch (exception e)
            {
                return false;
            }

            return true;
    }

    int MMCalibrationRWAPI::ReadFECustomData(uint8_t* buffer)
    {
        int status = DC_MM_SUCCESS;

        if (m_Rs400Device == nullptr)
            return DC_MM_ERROR_NOT_INITIALIZED;

        if (buffer == NULL)
        {
            return DC_MM_ERROR_INVALID_BUFFER;
        }

        static_assert(DC_MM_FE_CUSTOM_DATA_SIZE == DC_MM_FE_TABLE_SIZE - sizeof(MMTableHeader), "DC_MM_FE_CUSTOM_DATA_SIZE must be 16 bytes smaller than DC_MM_FE_TABLE_SIZE!");

        MMEEPROM eeprom;
        uint32_t* pArg = (uint32_t*) &MMER_Cmd[8];
        *pArg = 0;                     // offset to base of the eeprom
        pArg++;
        *pArg = sizeof(MMEEPROM);      // data size to full eeprom

        if (HwMonitorCmd_Get(m_Rs400Device, MMER_Cmd, eeprom.data, sizeof(MMEEPROM)))
        {
            // extract FE calibration table data content
            uint32_t contentSize = DC_MM_FE_CUSTOM_DATA_SIZE;
            uint8_t* content = ((uint8_t*)(eeprom.data)) + DC_MM_FE_TABLE_OFFSET + sizeof(MMTableHeader);

            DS_MEMCPY(buffer, content, contentSize);
        }
        else
        {
            status = DC_MM_ERROR_FAIL;
        }

        return status;
    }

    int MMCalibrationRWAPI::WriteFECustomData(uint8_t* buffer)
    {
        int status = DC_MM_SUCCESS;

        if (m_Rs400Device == nullptr)
            return DC_MM_ERROR_NOT_INITIALIZED;

        if (buffer == NULL)
        {
            return DC_MM_ERROR_INVALID_BUFFER;
        }

        MMEEPROM eeprom;
        memset((void*)&eeprom, 0, sizeof(eeprom));

        uint32_t* pArg = (uint32_t*)&MMER_Cmd[8];
        *pArg = 0;
        pArg++;
        *pArg = sizeof(MMEEPROM);

        if (HwMonitorCmd_Get(m_Rs400Device, MMER_Cmd, eeprom.data, sizeof(MMEEPROM)))
        {
            MMTableHeader* pHeader = NULL;
            uint32_t contentSize = 0;
            uint8_t* pContent = NULL;
            uint32_t crc32 = 0;

            // FE calibration table
            pHeader = (MMTableHeader*) ((uint8_t*)(eeprom.data) + DC_MM_FE_TABLE_OFFSET);
            pHeader->version   = 0x0001;
            pHeader->tableType = 0x0033;
            pHeader->tableSize = 0x00000088;
            pHeader->resrved   = 0xFFFFFFFF;

            contentSize = DC_MM_FE_CUSTOM_DATA_SIZE;
            pContent = ((uint8_t*)(eeprom.data)) + DC_MM_FE_TABLE_OFFSET + sizeof(MMTableHeader);

            DS_MEMCPY(pContent, buffer, contentSize);

            crc32 = CalculateCrc32(pContent, contentSize);
            pHeader->crc32 = crc32;

            // TM1 calibration model
            pHeader = (MMTableHeader*)((uint8_t*)(eeprom.data) + DC_MM_TM1_MODEL_OFFSET);
            pHeader->version   = 0x0001;
            pHeader->tableType = 0x0014;
            pHeader->tableSize = 0x000000B4;
            pHeader->resrved   = 0xFFFFFFFF;

            contentSize = pHeader->tableSize;
            pContent = ((uint8_t*)(eeprom.data)) + DC_MM_TM1_MODEL_OFFSET + sizeof(MMTableHeader);

            crc32 = CalculateCrc32(pContent, contentSize);
            pHeader->crc32 = crc32;

            // calibration table
            pHeader = (MMTableHeader*)((uint8_t*)(eeprom.data) + DC_MM_CAL_TABLE_OFFSET);
            pHeader->version   = 0x0002;
            pHeader->tableType = 0x0015;
            pHeader->tableSize = 0x00000198;
            pHeader->resrved   = 0xFFFFFFFF;

            contentSize = pHeader->tableSize;
            pContent = ((uint8_t*)(eeprom.data)) + DC_MM_CAL_TABLE_OFFSET + sizeof(MMTableHeader);

            crc32 = CalculateCrc32(pContent, contentSize);
            pHeader->crc32 = crc32;

            // mm eeprom
            uint16_t* pp = (uint16_t*)&MMEW_Cmd[8];
            *pp = sizeof(MMEEPROM);                      // data size

            contentSize = sizeof(eeprom) - sizeof(MMTableHeader);
            pContent = ((uint8_t*)(eeprom.data)) + sizeof(MMTableHeader);

            pHeader = (MMTableHeader*) eeprom.data;
            pHeader->version   = 0x0002;
            pHeader->tableType = 0x0007;
            pHeader->tableSize = 0x000001F8;
            pHeader->resrved   = 0xFFFFFFFF;

            crc32 = CalculateCrc32(pContent, contentSize);
            pHeader->crc32 = crc32;

            if (HwMonitorCmd_Set(m_Rs400Device, MMEW_Cmd, eeprom.data, sizeof(MMEEPROM)))
            {
                status = DC_MM_SUCCESS;
            }
            else
            {
                status = DC_MM_ERROR_FAIL;
            }
        }
        else
        {
            status = DC_MM_ERROR_FAIL;
        }

        return status;
    }

    int MMCalibrationRWAPI::ReadMMCustomData(uint8_t* buffer)
    {
        int status = DC_MM_SUCCESS;

        if (m_Rs400Device == nullptr)
            return DC_MM_ERROR_NOT_INITIALIZED;

        if (buffer == NULL)
        {
            return DC_MM_ERROR_INVALID_BUFFER;
        }

        static_assert(DC_MM_CUSTOM_DATA_SIZE == DC_MM_EEPROM_SIZE - sizeof(MMTableHeader), "DC_MM_IMU_CUSTOM_DATA_SIZE must be 16 bytes smaller than DC_MM_IMU_TABLE_SIZE!");

        MMEEPROM eeprom;
        uint32_t* pArg = (uint32_t*)&MMER_Cmd[8];
       *pArg = 0;                     // offset to base of the eeprom
        pArg++;
       *pArg = sizeof(MMEEPROM);      // data size to full eeprom

        if (HwMonitorCmd_Get(m_Rs400Device, MMER_Cmd, eeprom.data, sizeof(MMEEPROM)))
        {
            // extract FE calibration table data content
            uint32_t contentSize = DC_MM_CUSTOM_DATA_SIZE;
            uint8_t* content = ((uint8_t*)(eeprom.data)) + sizeof(MMTableHeader);

            DS_MEMCPY(buffer, content, contentSize);
        }
        else
        {
            status = DC_MM_ERROR_FAIL;
        }

        return status;
    }

    int MMCalibrationRWAPI::WriteMMCustomData(uint8_t* buffer)
    {
        int status = DC_MM_SUCCESS;

        if (m_Rs400Device == nullptr)
            return DC_MM_ERROR_NOT_INITIALIZED;

        if (buffer == NULL)
        {
            return DC_MM_ERROR_INVALID_BUFFER;
        }

        MMEEPROM eeprom;
        memset((void*)&eeprom, 0xFF, sizeof(eeprom));

        MMTableHeader* pHeader = NULL;
        uint32_t contentSize = 0;
        uint8_t* pContent = NULL;
        uint32_t crc32 = 0;

        // mm eeprom
        pHeader = (MMTableHeader*)eeprom.data;
        pHeader->version = 0x0002;
        pHeader->tableType = 0x0009;                // d435i
        pHeader->tableSize = DC_MM_CUSTOM_DATA_SIZE;
        pHeader->resrved = 0xFFFFFFFF;

        contentSize = sizeof(eeprom) - sizeof(MMTableHeader);
        pContent = ((uint8_t*)(eeprom.data)) + sizeof(MMTableHeader);

        DS_MEMCPY(pContent, buffer, contentSize);

        crc32 = CalculateCrc32(pContent, contentSize);
        pHeader->crc32 = crc32;

        uint16_t* pp = (uint16_t*)&MMEW_Cmd[8];
       *pp = sizeof(MMEEPROM);                      // data size

        if (HwMonitorCmd_Set(m_Rs400Device, MMEW_Cmd, eeprom.data, sizeof(MMEEPROM)))
        {
            status = DC_MM_SUCCESS;
        }
        else
        {
            status = DC_MM_ERROR_FAIL;
        }

        return status;
    }
}
