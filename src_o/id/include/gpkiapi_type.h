/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#if !defined(AFX_GPKIAPI_TYPE_H__E76E913A_2FF5_4CE9_8B2A_7E7EFBBEB24D__INCLUDED_)
#define AFX_GPKIAPI_TYPE_H__E76E913A_2FF5_4CE9_8B2A_7E7EFBBEB24D__INCLUDED_

typedef struct {
	unsigned char *pData;					/* 데이터의 포인터 */
	int nLen;								/* 데이터의 길이 */
} BINSTR;

#define IN									/* 입력 인자 */
#define OUT									/* 출력 인자 */

#define API_OPT_RSA_ENC_V15			0x01	/* RSA V1.5의 암.복호화 수행 */
#define API_OPT_RSA_ENC_V20			0x02	/* RSA V2.0의 암.복호화 수행 */
#define API_OPT_PADDING_TYPE_NONE	0x10	/* 대칭키 암.복호화 수행시 패딩하지 않음 */
#define API_OPT_PADDING_TYPE_PKCS5	0x20	/* 대칭키 암.복호화 수행시 PKCS5 패딩 수행 */

#define KEY_TYPE_PRIVATE			0x01	/* 개인키 */
#define KEY_TYPE_PUBLIC				0x02	/* 공개키 */
	
#define SYM_ALG_DES_CBC				0x10	/* DES CBC */
#define SYM_ALG_3DES_CBC			0x20 	/* 3DES CBC */
#define SYM_ALG_SEED_CBC			0x30	/* SEED CBC */
#define SYM_ALG_NEAT_CBC			0x40	/* NEAT CBC */
#define SYM_ALG_ARIA_CBC			0x50	/* ARIA CBC */
#define SYM_ALG_NES_CBC				0x60	/* NES CBC */

#define HASH_ALG_SHA1				0x01	/* SAH1 */
#define HASH_ALG_MD5				0x02	/* MD5 */
#define HASH_ALG_HAS160				0x03	/* HAS160 */
#define HASH_ALG_SHA256				0x04	/* SHA256 */

#define MAC_ALG_SHA1_HMAC			0x01	/* SAH1 HMAC */
#define MAC_ALG_MD5_HMAC			0x02	/* MD5 HMAC */

#define LDAP_DATA_CA_CERT			0x01	/* CA 인증서 */
#define LDAP_DATA_SIGN_CERT			0x02	/* 사용자 서명용 인증서 */
#define LDAP_DATA_KM_CERT			0x03	/* 사용자 암호화용 인증서 */
#define LDAP_DATA_ARL				0x05	/* CA 인증서 폐지 목록 */
#define LDAP_DATA_CRL				0x06	/* 사용자 인증서 폐지 목록 */
#define LDAP_DATA_DELTA_CRL			0x07	/* Delta 인증서 폐지 목록 */
#define LDAP_DATA_CTL				0x08	/* 인증서 신뢰 목록 */
#define LDAP_DATA_GPKI_WCERT		0x09	/* 행정 무선 인증체계의 인증서 */

#define MEDIA_TYPE_FILE_PATH		0x01	/* HardDisk, FloppyDisk, USBDriver, CD-KEY */
#define MEDIA_TYPE_DYNAMIC			0x02	/* 동적 저장매체(스마트카드 등) */

#define DATA_TYPE_OTHER				0x00	/* 임의 위치의 데이터 */
#define DATA_TYPE_GPKI_SIGN			0x01	/* 행정 서명용 인증서.개인키 */
#define DATA_TYPE_GPKI_KM			0x02	/* 행정 암호화용 인증서.개인키 */
#define DATA_TYPE_NPKI_SIGN			0x10	/* 공인 서명용 인증서.개인키 */
#define DATA_TYPE_NPKI_KM			0x20	/* 공인 암호화용 인증서.개인키 */

#define CERT_TYPE_SIGN				0x01	/* 서명용 */
#define CERT_TYPE_KM				0x02	/* 암호화용 */
#define CERT_TYPE_OCSP				0x03	/* OCSP 서버용 */
#define CERT_TYPE_TSA				0x04	/* TSA 서버용 */

#define CERT_VERIFY_FULL_PATH		0x01	/* 전체 경로 검증 */
#define CERT_VERIFY_CA_CERT			0x02	/* CA 인증서만 검증 */
#define CERT_VERIFY_USER_CERT		0x03	/* 사용자 인증서만 검증 */

#define CERT_REV_CHECK_ALL			0x01	/* ARL, CRL, OCSP 모두 확인 */
#define CERT_REV_CHECK_ARL			0x02	/* ARL 확인 */
#define CERT_REV_CHECK_CRL			0x04	/* CRL 확익 */
#define CERT_REV_CHECK_OCSP			0x08	/* OCSP 확인 */
#define CERT_REV_CHECK_NONE			0x10	/* 폐지여부 확인하지 않음 */

#endif /* !defined(AFX_GPKIAPI_TYPE_H__E76E913A_2FF5_4CE9_8B2A_7E7EFBBEB24D__INCLUDED_)  */
