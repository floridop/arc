#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>

#include "DelegationInterface.h"

int main(void) {
  std::string credentials(
"-----BEGIN CERTIFICATE-----\n"
"MIICJzCCAZCgAwIBAgIEUK3K0jANBgkqhkiG9w0BAQQFADBVMQ0wCwYDVQQKEwRH\n"
"cmlkMRIwEAYDVQQKEwlOb3JkdUdyaWQxDzANBgNVBAsTBnVpby5ubzEfMB0GA1UE\n"
"AxMWQWxla3NhbmRyIEtvbnN0YW50aW5vdjAeFw0wNzA5MDMyMTIwNDlaFw0wNzA5\n"
"MDQwOTI1NDlaMGoxDTALBgNVBAoTBEdyaWQxEjAQBgNVBAoTCU5vcmR1R3JpZDEP\n"
"MA0GA1UECxMGdWlvLm5vMR8wHQYDVQQDExZBbGVrc2FuZHIgS29uc3RhbnRpbm92\n"
"MRMwEQYDVQQDEwoxMzUzNTY2OTMwMFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBALgG\n"
"FqCe5+lqC0N22o0yePigj3lll8XudlXXuCrCHSe0xscrf4I1zY3Wh6u091smeFUG\n"
"8yzNnw0vkAJGgmcI5G0CAwEAAaMzMDEwDgYDVR0PAQH/BAQDAgWgMB8GCisGAQQB\n"
"m1ABgV4BAf8EDjAMMAoGCCsGAQUFBxUBMA0GCSqGSIb3DQEBBAUAA4GBAF3a+o6j\n"
"gzpCvFqcs0sW7yxOPUr52I4Xtg9sgsHtxC74LnbS0kfH2/IS5CYW18lzuNLTfBJf\n"
"ZcVOEGHBdtexxBr/of080wlp2FVSIOH4v6mwYTGdKNnI6CN08M4xzTnlN0cVNIm5\n"
"ZdQ8d565db1nVNO1GFUdP7ml7LYhMKqqFTdl\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIBPQIBAAJBALgGFqCe5+lqC0N22o0yePigj3lll8XudlXXuCrCHSe0xscrf4I1\n"
"zY3Wh6u091smeFUG8yzNnw0vkAJGgmcI5G0CAwEAAQJBALUuoWahLbpdgxt4ULPG\n"
"Jt67zqb6hJUHNJFOI/BNUEE/CzL213SUeazFdOGcSi+4wUQdG2YVf/gExyGGXnEl\n"
"rkECIQDaOvrbO7U84CAOfxBPpLCdNszo92ShWVbvnxR/z2VOMQIhANffiL7L0R+Q\n"
"wf4DLr9FfMJktwpZx7nHE9QJ38LONv79AiEAgmNdAOJC3lV3MdIfd8aJs9VLRyKR\n"
"YIoVlGQzBk5vU/ECIQCseCNAjIZfub/Dubc3icTLarvywRgZDTyCRAFKGodF4QIh\n"
"AL4/RnEwbdzxCfXdEe6WLjh6o8X/tVoXMKsuILM/m830\n"
"-----END RSA PRIVATE KEY-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIDQDCCAqmgAwIBAgICCjswDQYJKoZIhvcNAQEFBQAwTzENMAsGA1UEChMER3Jp\n"
"ZDESMBAGA1UEChMJTm9yZHVHcmlkMSowKAYDVQQDEyFOb3JkdUdyaWQgQ2VydGlm\n"
"aWNhdGlvbiBBdXRob3JpdHkwHhcNMDcwNTIxMTE0NjE0WhcNMDgwNTIwMTE0NjE0\n"
"WjBVMQ0wCwYDVQQKEwRHcmlkMRIwEAYDVQQKEwlOb3JkdUdyaWQxDzANBgNVBAsT\n"
"BnVpby5ubzEfMB0GA1UEAxMWQWxla3NhbmRyIEtvbnN0YW50aW5vdjCBnzANBgkq\n"
"hkiG9w0BAQEFAAOBjQAwgYkCgYEA1gaxDCuATqDJ9fGdPJfUs4LS37fnyV6XhsQU\n"
"SyXbbrTc1XEkYI/goWlR3HwkSlG5RF35r5bk/9zGtcOg3MF9CppqWdRGPTGPl9OS\n"
"l60wY+o9d5CyRo4xAUnUey60cWenUtuZAVPWqfPOeBrVCuX0qNQNPgMErJdTKm/G\n"
"30kKXBsCAwEAAaOCASMwggEfMAkGA1UdEwQCMAAwEQYJYIZIAYb4QgEBBAQDAgWg\n"
"MAsGA1UdDwQEAwIF4DAsBglghkgBhvhCAQ0EHxYdT3BlblNTTCBHZW5lcmF0ZWQg\n"
"Q2VydGlmaWNhdGUwHQYDVR0OBBYEFKYX6Nyfr6acgM+u+MKvIMjmhKgiMHcGA1Ud\n"
"IwRwMG6AFBgFwPwL0bc69GWSCftZoV/HiMTwoVOkUTBPMQ0wCwYDVQQKEwRHcmlk\n"
"MRIwEAYDVQQKEwlOb3JkdUdyaWQxKjAoBgNVBAMTIU5vcmR1R3JpZCBDZXJ0aWZp\n"
"Y2F0aW9uIEF1dGhvcml0eYIBADAsBgNVHREEJTAjgSFhbGVrc2FuZHIua29uc3Rh\n"
"bnRpbm92QGZ5cy51aW8ubm8wDQYJKoZIhvcNAQEFBQADgYEAEPzQ1JvLZ/viCC/U\n"
"YVD5pqMhBvcoTxO3vTFXfNJH0pRKtxHZ4Ib0Laef+7A0nX2Ct9j7npx0ekrIxwZR\n"
"8FUF0gGhoSrtQ1ciSYZmp+lkOT3dmWWWR8Gw72EByWH354F7Bpk0sOE4Ctc8bWtg\n"
"ggm88pXJ31uRDOFqP2S4VMB3vtA=\n"
"-----END CERTIFICATE-----\n"
  );
  std::string s;
  Arc::DelegationConsumer c;
  c.Backup(s);
  std::cerr<<"Private key:\n"<<s<<std::endl;
  c.Request(s);
  std::cerr<<"Request:\n"<<s<<std::endl;
  Arc::DelegationProvider p(credentials);
  s=p.Delegate(s);
  std::cerr<<"Delegation:\n"<<s<<std::endl;
  return 0;
}

