#pragma once

/**
 * @brief      for user add default GATT and GAP service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addCoreGroup(void);

/**
 * @brief      for user remove default GATT and GAP service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeCoreGroup(void);

/**
 * @brief      for user calculate database hash value(core version >= 5.1).
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_calculateDatabaseHash(void);

/**
 * @brief     for user set device name.
 * @param[in] name: user device name.
 * @return    none.
 */
void blc_svc_setDeviceName(const char *name);

/**
 * @brief     for user set appearance.
 * @param[in] appearance: user appearance.
 * @return    none.
 */
void blc_svc_setAppearance(uint16_t appearance);
