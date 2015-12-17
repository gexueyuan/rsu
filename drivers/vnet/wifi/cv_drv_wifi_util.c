#include "ieee80211.h"
#include "platform.h"
#include "nl80211.h"
#include "cv_drv_wifi.h"

void monitor_buf_print(uint8_t *pbuf , uint32_t len)
{
    uint32_t i , j=0 ,k;

    printf("================================================");
    for(i = 0 ; i < len ; i++) {
        if(i%16 == 0) {
            j = 0;
            printf("\r\n");
        }
        printf("%02x ",*((uint8_t *)(pbuf+i)));

        if(++j == 8)
            printf(" ");
    }

    printf("\r\n\n");
    return;
}

void mac_addr_n2a(char *mac_addr, unsigned char *arg)
{
	int i, l;

	l = 0;
	for (i = 0; i < ETH_ALEN ; i++) {
		if (i == 0) {
			sprintf(mac_addr+l, "%02x", arg[i]);
			l += 2;
		} else {
			sprintf(mac_addr+l, ":%02x", arg[i]);
			l += 3;
		}
	}
}

int mac_addr_a2n(unsigned char *mac_addr, char *arg)
{
	int i;

	for (i = 0; i < ETH_ALEN ; i++) {
		int temp;
		char *cp = strchr(arg, ':');
		if (cp) {
			*cp = 0;
			cp++;
		}
		if (sscanf(arg, "%x", &temp) != 1)
			return -1;
		if (temp < 0 || temp > 255)
			return -1;

		mac_addr[i] = temp;
		if (!cp)
			break;
		arg = cp;
	}
	if (i < ETH_ALEN - 1)
		return -1;

	return 0;
}

int parse_hex_mask(char *hexmask, unsigned char **result, size_t *result_len,
		   unsigned char **mask)
{
	size_t len = strlen(hexmask) / 2;
	unsigned char *result_val;
	unsigned char *result_mask = NULL;

	int pos = 0;

	*result_len = 0;

	result_val = calloc(len + 2, 1);
	if (!result_val)
		goto error;
	*result = result_val;
	if (mask) {
		result_mask = calloc(DIV_ROUND_UP(len, 8) + 2, 1);
		if (!result_mask)
			goto error;
		*mask = result_mask;
	}

	while (1) {
		char *cp = strchr(hexmask, ':');
		if (cp) {
			*cp = 0;
			cp++;
		}

		if (result_mask && (strcmp(hexmask, "-") == 0 ||
				    strcmp(hexmask, "xx") == 0 ||
				    strcmp(hexmask, "--") == 0)) {
			/* skip this byte and leave mask bit unset */
		} else {
			int temp, mask_pos;
			char *end;

			temp = strtoul(hexmask, &end, 16);
			if (*end)
				goto error;
			if (temp < 0 || temp > 255)
				goto error;
			result_val[pos] = temp;

			mask_pos = pos / 8;
			if (result_mask)
				result_mask[mask_pos] |= 1 << (pos % 8);
		}

		(*result_len)++;
		pos++;

		if (!cp)
			break;
		hexmask = cp;
	}

	return 0;
 error:
	free(result_val);
	free(result_mask);
	return -1;
}

unsigned char *parse_hex(char *hex, size_t *outlen)
{
	unsigned char *result;

	if (parse_hex_mask(hex, &result, outlen, NULL))
		return NULL;
	return result;
}

int ieee80211_channel_to_frequency(int chan, enum nl80211_band band)
{
	/* see 802.11 17.3.8.3.2 and Annex J
	 * there are overlapping channel numbers in 5GHz and 2GHz bands */
	if (chan <= 0)
		return 0; /* not supported */
	switch (band) {
	case NL80211_BAND_2GHZ:
		if (chan == 14)
			return 2484;
		else if (chan < 14)
			return 2407 + chan * 5;
		break;
	case NL80211_BAND_5GHZ:
		if (chan >= 182 && chan <= 196)
			return 4000 + chan * 5;
		else
			return 5000 + chan * 5;
		break;
	case NL80211_BAND_60GHZ:
		if (chan < 5)
			return 56160 + chan * 2160;
		break;
	default:
		;
	}
	return 0; /* not supported */
}

int ieee80211_frequency_to_channel(int freq)
{
	/* see 802.11-2007 17.3.8.3.2 and Annex J */
	if (freq == 2484)
		return 14;
	else if (freq < 2484)
		return (freq - 2407) / 5;
	else if (freq >= 4910 && freq <= 4980)
		return (freq - 4000) / 5;
	else if (freq <= 45000) /* DMG band lower limit */
		return (freq - 5000) / 5;
	else if (freq >= 58320 && freq <= 64800)
		return (freq - 56160) / 2160;
	else
		return 0;
}

static int hex2num(char digit)
{
	if (!isxdigit(digit))
		return -1;
	if (isdigit(digit))
		return digit - '0';
	return tolower(digit) - 'a' + 10;
}

static int hex2byte(char *hex)
{
	int d1, d2;

	d1 = hex2num(hex[0]);
	if (d1 < 0)
		return -1;
	d2 = hex2num(hex[1]);
	if (d2 < 0)
		return -1;
	return (d1 << 4) | d2;
}

static char *hex2bin(char *hex, char *buf)
{
	char *result = buf;
	int d;

	while (hex[0]) {
		d = hex2byte(hex);
		if (d < 0)
			return NULL;
		buf[0] = d;
		buf++;
		hex += 2;
	}

	return result;
}


void iw_hexdump(const char *prefix, const __u8 *buf, size_t size)
{
	int i;

	printf("%s: ", prefix);
	for (i = 0; i < size; i++) {
		if (i && i % 16 == 0)
			printf("\n%s: ", prefix);
		printf("%02x ", buf[i]);
	}
	printf("\n\n");
}
