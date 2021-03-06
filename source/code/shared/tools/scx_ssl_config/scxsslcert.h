/*----------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
*/
/**
   \file
   
   \brief      Defines the openSSL certificate generation class
   
   \date       1-29-2008
   
   Wraps the openSSL certificate generation functions for use with the SCX
   installation tools.
   
*/

#ifndef SCXSSLCERT_H
#define SCXSSLCERT_H

#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxexception.h>
#include <iosfwd>
#include <openssl/x509.h>
#include <iostream>
#include <sstream>
#include <set>
/*----------------------------------------------------------------------------*/
/**
   Generic exception for SSL Certificate errors.
    
*/ 
class SCXSSLException : public SCXCoreLib::SCXException {
 public:
    /*----------------------------------------------------------------------------*/
    /**
       Ctor
       \param[in] reason       Cause of the exception.
       \param[in] l            Source code location object
    */
    SCXSSLException(std::wstring reason,
                    const SCXCoreLib::SCXCodeLocation& l) :
        SCXException(l),
        m_Reason(reason)
    { };

    std::wstring What() const;

 protected:
    //! The source code name of the violating pointer
    std::wstring   m_Reason;
};

/*----------------------------------------------------------------------------*/
/**
   openSSL certificate provides a wrapper class for the openSSL certificate
   generation functons.
   
   \author Carl Nicol
   \date 1-31-2008

   SCXSSLCertificate wraps the openSSL API calls used to generate certificates.

   \note Data is stored using wstring, however, the openSSL API calls use C-strings
   so all data is translated to a C-string before it is actually used.

*/
class SCXSSLCertificate
{
 private:
    /// The type of encoding used to produce the key.
    enum KeyType {
        eKeyTypeNone = 0,       ///< No encoding type specified.
        eKeyTypeRSA  = 1,       ///< RSA encoding
        eKeyTypeDSA  = 2,       ///< DSA encoding
        eKeyTypeDH   = 3,       ///< DH encoding
        eKeyTypeEC   = 4,       ///< EC encoding
        eKeyTypeMax  = 5        ///< Above range of encoding enums.
    };

    /// The certificate file format type
    enum FormatType {
        eFormatTypeNone = 0,    ///< No certificate format specified.
        eFormatTypeASN1 = 1,    ///< ASN1 formatted certificate.
        eFormatTypePEM  = 3,    ///< PEM formatted certificate
        eFormatTypeMax  = 4     ///< Above range of format types.
    };

    int m_startDays;                     ///< Days to offset valid start time with;
    int m_endDays;                       ///< Days to offset valid end time with;
    int m_bits;                          ///< Number of bits in key
    bool m_clientCert;                   ///<  Certificate to be used for client authentication

protected:
    SCXCoreLib::SCXFilePath m_KeyPath;   ///< Path to key file;
    SCXCoreLib::SCXFilePath m_CertPath;  ///< Path to certificate;
    std::wstring m_hostname;             ///< Hostname
    std::wstring m_domainname;           ///< Domainname

public:
    SCXSSLCertificate(SCXCoreLib::SCXFilePath keyPath, SCXCoreLib::SCXFilePath certPath,
                      int startDays, int endDays, const std::wstring & hostname,
                      const std::wstring & domainname, int bits, bool clientCert = false);
    
    virtual ~SCXSSLCertificate();

    void LoadRndNumber();
    void SaveRndNumber();
    void Generate();

protected:
    size_t LoadRandomFromFile(const char* file, size_t num);

private:
    /// Do not allow copy. Intentionally left unimplemented.
    SCXSSLCertificate( const SCXSSLCertificate & rhs );
    /// Do not allow copy. Intentionally left unimplemented.
    SCXSSLCertificate & operator=( const SCXSSLCertificate & rhs );

    void DoGenerate();
    void SetX509Properties(X509_REQ *req, EVP_PKEY *pkey);
    virtual size_t LoadRandomFromDevRandom(size_t randomNeeded);
    virtual size_t LoadRandomFromDevUrandom(size_t randomNeeded);
    virtual size_t LoadRandomFromUserFile();
    virtual void DisplaySeedWarning(size_t goodRandomNeeded);

    friend class ScxSSLCertTest;
};

// Comparator for SuffixSortedFileSet
struct IntegerSuffixComparator
{
    bool operator()(const SCXCoreLib::SCXFilePath * pa, const SCXCoreLib::SCXFilePath * pb) const;
    static bool IsGoodFileName(const SCXCoreLib::SCXFilePath& path);
};

// Set sorted on integer file name suffix, e.g. libcidn.so.<N> .
typedef std::set<const SCXCoreLib::SCXFilePath *, IntegerSuffixComparator> SuffixSortedFileSet;

/**
   SCXSSLCertificateLocalDomain is a wrapper to SCXSSLCertificate that provides 
   conversion of localized domain names. 
*/

typedef int(*IDNFuncPtr)(const char *, char **, int) ;

class SCXSSLCertificateLocalizedDomain : public SCXSSLCertificate
{
public:
    SCXSSLCertificateLocalizedDomain(SCXCoreLib::SCXFilePath keyPath, SCXCoreLib::SCXFilePath certPath,
                              int startDays, int endDays, const std::wstring & hostname,
                                     const std::wstring & domainname_raw, int bits, bool clientCert = false);

    using SCXSSLCertificate::Generate; 
    void Generate(std::ostringstream& verbage);

private:
    static void * GetLibIDN(void);
    static void CloseLibIDN(void * hLib);
    static void * GetLibIDNByDirectory(const char * sDir);
    static IDNFuncPtr GetIDNAToASCII(void * hLib);
    static void CleanupErrorOutput(const char * sErr, std::ostringstream& verbage);

private:
    /**
       Helper class to ensure that IDN library is closed properly regardless of exceptions
    */
    class AutoClose 
    {
    public:
    /*
      AutoClose constructor
      \param[in]    hLib      Library handle to close
    */
    AutoClose(void * hLib) : m_hLib(hLib) {}
    ~AutoClose();

    private:
        void * m_hLib;     //!< Library handle
    };

private:
    std::wstring m_domainname_raw;

    friend class ScxSSLCertTest;
};

/*----------------------------------------------------------------------------*/
/**
   Specific errno exception for username related errors (see SCXErrnoException).
*/
class SCXErrnoUserNameException : public SCXCoreLib::SCXErrnoException
{
public:
    /*----------------------------------------------------------------------------*/
    /**
       Ctor
       \param[in] fkncall Function call for user-related operation
       \param[in] user    username parameter causing internal error
       \param[in] errno_  System error code with local interpretation
       \param[in] l       Source code location object
    */

    SCXErrnoUserNameException(std::wstring fkncall, std::wstring user, int errno_, const SCXCoreLib::SCXCodeLocation& l)
        : SCXErrnoException(fkncall, errno_, l), m_fkncall(fkncall), m_user(user)
    { };

    std::wstring What() const {
        std::wostringstream txt;
        txt << L"Calling " << m_fkncall << "() with user name parameter\"" << m_user
            << "\", returned an error with errno = " << m_errno << L" (" << m_errtext.c_str() << L")";
        return txt.str();
    }

    /** Returns function call for the user operation falure
        \returns user-operation in std::wstring encoding
    */
    std::wstring GetFnkcall() const { return m_fkncall; }

    std::wstring GetUser() const { return m_user; }
protected:
    //! Text of user-related function call
    std::wstring m_fkncall;
    std::wstring m_user;
};


#endif /* SCXSSLCERT_H */

/*--------------------------E-N-D---O-F---F-I-L-E----------------------------*/

