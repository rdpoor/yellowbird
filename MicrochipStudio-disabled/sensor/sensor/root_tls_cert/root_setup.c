/**
 *
 * \file
 *
 * \brief Root certificate parser and downloader
 *
 * Copyright (c) 2017-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
INCLUDESS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#include "crypto/crypto_api.h"
#include "root_setup.h"
#include "programmer/programmer.h"
#include "pem.h"

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
MACROS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#define ROOT_CERT_FLASH_START_PATTERN_LENGTH		16

#define ROOT_CERT_FLASH_EMPTY_PATTERN \
{\
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF	\
}
	
/* tstrRootCertEntryHeader Format used in 19.4.x */
#define ROOT_CERT_FLASH_START_PATTERN_V0	\
{\
	0x01, 0xF1, 0x02, 0xF2, 0x03, 0xF3, 0x04, 0xF4,	\
	0x05, 0xF5, 0x06, 0xF6, 0x07, 0xF7, 0x08, 0xF8	\
}

/* tstrRootCertEntryHeader Format used in 19.5.x */
#define ROOT_CERT_FLASH_START_PATTERN	\
{\
	0x11, 0xF1, 0x12, 0xF2, 0x13, 0xF3, 0x14, 0xF4, \
	0x15, 0xF5, 0x16, 0xF6, 0x17, 0xF7, 0x18, 0xF8	\
}

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
DATA TYPES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

typedef struct{
	uint8	au8StartPattern[ROOT_CERT_FLASH_START_PATTERN_LENGTH];
	uint32	u32nCerts;
}tstrRootCertFlashHeader;

typedef enum{
	ROOT_CERT_PUBKEY_RSA		= 1,
	ROOT_CERT_PUBKEY_ECDSA		= 2
}tenuRootCertPubKeyType;

typedef struct{
	uint16	u16NSz;
	uint16	u16ESz;
}tstrRootCertRsaKeyInfo;

typedef struct{
	uint16	u16CurveID;
	uint16	u16KeySz;
}tstrRootCertEcdsaKeyInfo;

typedef struct{
	uint32	u32PubKeyType;
	union{
		tstrRootCertRsaKeyInfo		strRsaKeyInfo;
		tstrRootCertEcdsaKeyInfo	strEcsdaKeyInfo;
	};
}tstrRootCertPubKeyInfo;


/*!
@struct
	tstrRootCertEntryHeader

@brief
	Header of a root certificate entry in flash.
*/	
typedef struct{
	uint8					au8SHA1NameHash[CRYPTO_SHA1_DIGEST_SIZE];
	tstrSystemTime			strStartDate;
	tstrSystemTime			strExpDate;
	tstrRootCertPubKeyInfo	strPubKey;
}tstrRootCertEntryHeader;

void UTILITY_PrintBuffer(unsigned char* buf,int len, unsigned char mode)
{
    int  i = 0x00 ;

    printf(" Buffer Length To Print = %d BUFFER = %p\n  ",len,buf);

    for( i=0; i<len; i++)
    {
        if ( mode == 0x00 )
        {
            printf("%x ", buf[i]);
        }
        else
        {
            printf("%c\t", buf[i]);
        }

        if( ((i+1) % 16) == 0x00)
        {
            printf("\n");
        }
    }
    printf("\n ");
}
/**********************************************************************
Function
	GetRootCertificate

Description
	Get the root certificates from WINC SPI flash

Return
	Status
***********************************************************************/
static int GetRootCertificate(uint8 *pu8RootCert, uint32 u32RootCertSz, txtrX509CertInfo *pstrX509)
{
	int		ret;
	uint32	u32FileSize;
	uint8	*pcRootCertDER;
printf("%s() Entry \n",__FUNCTION__);
	/* Decode the certificate.
	*/
	ret = DecodeX509Certificate(pu8RootCert, u32RootCertSz, &pcRootCertDER, &u32FileSize);
	if(ret == M2M_SUCCESS)
	{
		UTILITY_PrintBuffer( pcRootCertDER,u32FileSize, 0x00);
		ret = CryptoX509CertDecode(pcRootCertDER, (uint16)u32FileSize, pstrX509, 0);
		if(ret == M2M_SUCCESS)
		{
			printf("\r\n>>>Found Certificate:\n");
			printf(">>>\t%s\n", pstrX509->strSubject.acCmnName);
		}
	}
	else
	{
		printf("\r\n>>>Invalid certificate!\n");
	}
	printf("%s() Exit \n",__FUNCTION__);
	return ret;
}

/**********************************************************************
Function
	writeRootCertEntry

Description
	Adds the new root certificate to the buffer

Return
	Size of the certificate
***********************************************************************/
static uint16 writeRootCertEntry(uint8 *pu8WriteBuff, txtrX509CertInfo *pstrRootCert)
{
	uint16 u16WriteSz = 0;

	if((pu8WriteBuff != NULL) && (pstrRootCert != NULL))
	{
		tstrRootCertEntryHeader		*pstrEntryHdr	= (tstrRootCertEntryHeader*)pu8WriteBuff;
		uint8						*pu8KeyMem		= &pu8WriteBuff[sizeof(tstrRootCertEntryHeader)];

		u16WriteSz = sizeof(tstrRootCertEntryHeader);
		
		m2m_memset((uint8*)pstrEntryHdr, 0, sizeof(tstrRootCertEntryHeader));

		/* 
			Write Root Certificate Entry Header
		*/
		m2m_memcpy(pstrEntryHdr->au8SHA1NameHash, pstrRootCert->strSubject.au8NameSHA1, CRYPTO_SHA1_DIGEST_SIZE);		// Subject Name SHA1
		m2m_memcpy((uint8*)&pstrEntryHdr->strStartDate, (uint8*)&pstrRootCert->strStartDate, sizeof(tstrSystemTime));	// Cert. Start Date.
		m2m_memcpy((uint8*)&pstrEntryHdr->strExpDate, (uint8*)&pstrRootCert->strExpiryDate, sizeof(tstrSystemTime));	// Cert. Expiration Date.

		/*
			Write the certificate public key
		*/
		if(pstrRootCert->strPubKey.enuCertKeyType == X509_CERT_PUBKEY_RSA)
		{
			/*
				RSA Public Key
			*/
			tstrRSAPubKey	*pstrKey = &pstrRootCert->strPubKey.strRsaPub;

			pstrEntryHdr->strPubKey.u32PubKeyType			= ROOT_CERT_PUBKEY_RSA;
			pstrEntryHdr->strPubKey.strRsaKeyInfo.u16NSz	= pstrKey->u16NSize;
			pstrEntryHdr->strPubKey.strRsaKeyInfo.u16ESz	= pstrKey->u16ESize;

			/* N */
		//	m2m_memset(pu8KeyMem,0x00,WORD_ALIGN(pstrKey->u16ESize) + WORD_ALIGN(pstrKey->u16NSize));
			m2m_memcpy(pu8KeyMem, pstrKey->pu8N, pstrKey->u16NSize);
			pu8KeyMem += pstrKey->u16NSize;
UTILITY_PrintBuffer(pstrKey->pu8N,pstrKey->u16NSize, 0x00);
			/* E */
			m2m_memcpy(pu8KeyMem, pstrKey->pu8E, pstrKey->u16ESize);
			u16WriteSz += WORD_ALIGN(pstrKey->u16ESize) + WORD_ALIGN(pstrKey->u16NSize);

UTILITY_PrintBuffer( pstrKey->pu8E,pstrKey->u16ESize, 0x00);
printf( "sizes pstrKey->u16NSize = %d  WORD = %d \n",pstrKey->u16NSize,WORD_ALIGN(pstrKey->u16NSize));
printf( "sizes pstrKey->u16ESize = %d  WORD = %d \n",pstrKey->u16ESize,WORD_ALIGN(pstrKey->u16ESize));
UTILITY_PrintBuffer(&pu8WriteBuff[sizeof(tstrRootCertEntryHeader)],pstrKey->u16NSize+pstrKey->u16ESize, 0x00);
		}
		else if(pstrRootCert->strPubKey.enuCertKeyType == X509_CERT_PUBKEY_ECDSA)
		{
			tstrECDSAPubKey	*pstrKey = &pstrRootCert->strPubKey.strEcdsaPub;

			pstrEntryHdr->strPubKey.u32PubKeyType				= ROOT_CERT_PUBKEY_ECDSA;
			pstrEntryHdr->strPubKey.strEcsdaKeyInfo.u16CurveID	= pstrKey->u16CurveID;
			pstrEntryHdr->strPubKey.strEcsdaKeyInfo.u16KeySz	= pstrKey->u16EcPointSz;
			m2m_memcpy(pu8KeyMem, pstrKey->au8EcPoint, pstrKey->u16EcPointSz * 2);
			u16WriteSz += pstrKey->u16EcPointSz * 2;
		}
	}
	return u16WriteSz;
}

/**********************************************************************
Function
	UpdateRootList

Description
	Update the root certificate list

Return
	Status
***********************************************************************/
static sint8 UpdateRootList(txtrX509CertInfo *pstrRootCert)
{
	uint32					u32Idx;
	uint8					bIncrement			= 0;
	uint32					u32nStoredCerts		= 0;
	uint8					au8StartPattern[]	= ROOT_CERT_FLASH_START_PATTERN;
	uint8                   au8EmptyPattern[]	= ROOT_CERT_FLASH_EMPTY_PATTERN;
	tstrRootCertFlashHeader	*pstrRootFlashHdr;
	tstrRootCertEntryHeader	*pstrEntryHdr;
	uint16					u16Offset;
	uint16					u16WriteSize;
	tstrRootCertPubKeyInfo	*pstrKey;

	pstrRootFlashHdr	= (tstrRootCertFlashHeader*)((void *)gau8CertMem);
	u16Offset			= sizeof(tstrRootCertFlashHeader);

	/* Check if the flash has been written before.
	*/
	if(m2m_memcmp(au8EmptyPattern, pstrRootFlashHdr->au8StartPattern, ROOT_CERT_FLASH_START_PATTERN_LENGTH) != 0)
	{
		u32nStoredCerts = pstrRootFlashHdr->u32nCerts;
		bIncrement = 1;
		printf("No of Stored Certificates = %d\n", pstrRootFlashHdr->u32nCerts);
		for(u32Idx = 0 ; u32Idx < u32nStoredCerts ; u32Idx ++)
		{
			pstrEntryHdr	= (tstrRootCertEntryHeader*)((void *)&gau8CertMem[u16Offset]);
			pstrKey			= &pstrEntryHdr->strPubKey;

			/* Check for match (equivalent NameSHA1). 
			*/
			if(!m2m_memcmp(pstrRootCert->strSubject.au8NameSHA1, pstrEntryHdr->au8SHA1NameHash, 20))
			{
				/* The current entry will be overwritten.
				*/
				printf("Current ENTRY over written = %d \n",u32Idx);
				bIncrement = 0;
				break;
			}
			u16Offset += sizeof(tstrRootCertEntryHeader);
			u16Offset += (pstrKey->u32PubKeyType == ROOT_CERT_PUBKEY_RSA) ? 
				(WORD_ALIGN(pstrKey->strRsaKeyInfo.u16NSz) + WORD_ALIGN(pstrKey->strRsaKeyInfo.u16ESz)) : (WORD_ALIGN(pstrKey->strEcsdaKeyInfo.u16KeySz) * 2);
		}
	}
	else
	{
		printf("No prior Certificates  \n ");	
		pstrRootFlashHdr->u32nCerts = 0;
		m2m_memcpy(pstrRootFlashHdr->au8StartPattern, au8StartPattern, ROOT_CERT_FLASH_START_PATTERN_LENGTH);
		bIncrement = 1;
	}
	printf("Offset to write new entry  = %d bIncrement = %d\n ", u16Offset,bIncrement);
	u16WriteSize = writeRootCertEntry(&gau8CertMem[u16Offset], pstrRootCert);

	if(bIncrement)
	{
		/* A new certificate is to be inserted into the flash.
		Increment the number of stored Certificates. 
		*/
		if(u16Offset + u16WriteSize > M2M_TLS_ROOTCER_FLASH_SIZE)
		{
			printf("(ERROR) Root Certificate Flash is Full\n");
			return -1;
		}
		pstrRootFlashHdr->u32nCerts ++;
	}
	return 0;
}

/**********************************************************************
*	@fn			int WriteRootCertificate
*	@brief		Write a given Root Certificate on the WINC1500 FLASH Root certificate Store. 
*	@param[in]	pu8RootCert
*					Buffer hoilding the root certificate (Both DER and PEM formats are valid).
*
*   @param [in]	u32RootCertSz
*					Size of the root certificate buffer
*
*	@return		writing status
**********************************************************************/
int WriteRootCertificate(uint8 *pu8RootCert, uint32 u32RootCertSz)
{
	txtrX509CertInfo	strX509Root;
	int					ret = -1;

	/* Read Certificate File.
	*/ 
	
	memset(gau8CertMem, 0, M2M_TLS_ROOTCER_FLASH_SIZE);
	if(GetRootCertificate(pu8RootCert, u32RootCertSz, &strX509Root) == 0)
	{
		programmer_read_root_cert(gau8CertMem);
		if(UpdateRootList(&strX509Root) == 0)
		{
#if 1 //wdb 			
			/* Erase memory. 
			*/
		printf("Updating Certificate\n");	
			ret = programmer_erase_root_cert();
			if(M2M_SUCCESS != ret) goto END;
			/* Write.
			*/
			printf("\r\n>> Writing the Root Certificate to SPI flash...\r\n");
			ret = programmer_write_root_cert(gau8CertMem);
			if(M2M_SUCCESS != ret) goto END;
			printf("--- Root Certificate written to SPI flash ---\r\n\r\n");
			nm_bsp_sleep(50);
#endif
#ifdef ENABLE_VERIFICATION //Enable verification or print array

			{
				uint32			u32Idx;

				memset(gau8Verify, 0, M2M_TLS_ROOTCER_FLASH_SIZE);
				programmer_read_root_cert(gau8Verify);

				for(u32Idx = 0; u32Idx < M2M_TLS_ROOTCER_FLASH_SIZE; u32Idx ++)
				{
					if(gau8CertMem[u32Idx] != gau8Verify[u32Idx])
					{
						printf(">> ERROR Root certificate verification failed\r\n");
						ret = -1;
						break;
					}
				}
				printf(">> Root Certificates Verified OK\r\n");
			}

#endif //Enable verification or print array
		printf("FINAL HERE\n"); 
		}
		CryptoX509DeleteContext(&strX509Root);
	}
END:
	return ret;
}




void read_check_certificates()
{	
	uint32					u32Idx;
	uint8					bIncrement			= 0;
	uint32					u32nStoredCerts		= 0;
	uint8					au8StartPattern[]	= ROOT_CERT_FLASH_START_PATTERN;
	uint8                   au8EmptyPattern[]	= ROOT_CERT_FLASH_EMPTY_PATTERN;
	tstrRootCertFlashHeader	*pstrRootFlashHdr;
	tstrRootCertEntryHeader	*pstrEntryHdr;
	uint16					u16Offset;
	uint16					u16WriteSize;
	tstrRootCertPubKeyInfo	*pstrKey;
	uint8                       *pu8KeyMem;
	printf("%s() Entry txtrX509CertInfo size = %d\n",__FUNCTION__,sizeof(txtrX509CertInfo));
	pstrRootFlashHdr	= (tstrRootCertFlashHeader*)((void *)gau8CertMem);
	u16Offset			= sizeof(tstrRootCertFlashHeader);
	memset(gau8CertMem,0x00,M2M_TLS_ROOTCER_FLASH_SIZE);
	programmer_read_root_cert(gau8CertMem);	
	/* Check if the flash has been written before.
	*/
	if(m2m_memcmp(au8EmptyPattern, pstrRootFlashHdr->au8StartPattern, ROOT_CERT_FLASH_START_PATTERN_LENGTH) != 0)
	{
		printf("START SEQUENCE Matched\n");
		
		u32nStoredCerts = pstrRootFlashHdr->u32nCerts;
		printf("No of Stored Certificates = %d\n", pstrRootFlashHdr->u32nCerts);
		for(u32Idx = 0 ; u32Idx < u32nStoredCerts ; u32Idx ++)
		{
			pstrEntryHdr	= (tstrRootCertEntryHeader*)((void *)&gau8CertMem[u16Offset]);
			pstrKey			= &pstrEntryHdr->strPubKey;
            pu8KeyMem      = &gau8CertMem[u16Offset+sizeof(tstrRootCertEntryHeader)];
		//	printf("Certificate = %d ,  au8SHA1NameHash = %.*s , key_type = %d \n",u32Idx,20,pstrEntryHdr->au8SHA1NameHash,pstrEntryHdr->strPubKey.u32PubKeyType);
			
			printf("Certificate = %d ,  start year=%d  start month = %d start date  = %d, key_type = %d \n",u32Idx,pstrEntryHdr->strStartDate.u16Year,pstrEntryHdr->strStartDate.u8Month, pstrEntryHdr->strStartDate.u8Day, pstrEntryHdr->strPubKey.u32PubKeyType);
			u16Offset += sizeof(tstrRootCertEntryHeader);
			u16Offset += (pstrKey->u32PubKeyType == ROOT_CERT_PUBKEY_RSA) ?
			(WORD_ALIGN(pstrKey->strRsaKeyInfo.u16NSz) + WORD_ALIGN(pstrKey->strRsaKeyInfo.u16ESz)) : (WORD_ALIGN(pstrKey->strEcsdaKeyInfo.u16KeySz) * 2);
			int size = (pstrKey->u32PubKeyType == ROOT_CERT_PUBKEY_RSA) ?
			(WORD_ALIGN(pstrKey->strRsaKeyInfo.u16NSz) + WORD_ALIGN(pstrKey->strRsaKeyInfo.u16ESz)) : (WORD_ALIGN(pstrKey->strEcsdaKeyInfo.u16KeySz) * 2);
		printf( "sizes pstrKey->u16NSize = %d  WORD = %d \n",pstrKey->strRsaKeyInfo.u16NSz,WORD_ALIGN(pstrKey->strRsaKeyInfo.u16NSz));
		printf( "sizes pstrKey->u16ESize = %d  WORD = %d \n",pstrKey->strRsaKeyInfo.u16ESz,WORD_ALIGN(pstrKey->strRsaKeyInfo.u16ESz));	
			UTILITY_PrintBuffer(pu8KeyMem,size,0x00);
				//		UTILITY_PrintBuffer(pstrEntryHdr,u16Offset,0x00);
		}
	}
}