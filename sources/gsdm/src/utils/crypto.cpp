/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */


#include "utils/crypto.h"
#include "logging/logger.h"


namespace gsdm {


DHWrapper::DHWrapper(int32_t bits_count) {
	bits_count_ = bits_count;
	dh_ = NULL;
	shared_key_ = NULL;
	shared_key_length_ = 0;
	peer_public_key_ = NULL;
}

DHWrapper::~DHWrapper() {
	Cleanup();
}

bool DHWrapper::Initialize() {
	Cleanup();

	//1. Create the DH
	dh_ = DH_new();
	if (dh_ == NULL) {
		FATAL("Unable to create DH");
		Cleanup();
		return false;
	}

	//2. Create his internal p and g
	dh_->p = BN_new();
	if (dh_->p == NULL) {
		FATAL("Unable to create p");
		Cleanup();
		return false;
	}
	dh_->g = BN_new();
	if (dh_->g == NULL) {
		FATAL("Unable to create g");
		Cleanup();
		return false;
	}

	//3. initialize p, g and key length
	if (BN_hex2bn(&dh_->p, P1024) == 0) {
		FATAL("Unable to parse P1024");
		Cleanup();
		return false;
	}
	if (BN_set_word(dh_->g, 2) != 1) {
		FATAL("Unable to set g");
		Cleanup();
		return false;
	}

	//4. Set the key length
	dh_->length = bits_count_;

	//5. Generate private and public key
	if (DH_generate_key(dh_) != 1) {
		FATAL("Unable to generate DH public/private keys");
		Cleanup();
		return false;
	}

	return true;
}

bool DHWrapper::CopyPublicKey(uint8_t *dst, int32_t dst_length) {
	if (dh_ == NULL) {
		FATAL("DHWrapper not initialized");
		return false;
	}

	return CopyKey(dh_->pub_key, dst, dst_length);
}

bool DHWrapper::CopyPrivateKey(uint8_t *dst, int32_t dst_length) {
	if (dh_ == NULL) {
		FATAL("DHWrapper not initialized");
		return false;
	}

	return CopyKey(dh_->priv_key, dst, dst_length);
}

bool DHWrapper::CreateSharedKey(uint8_t *peer_public_key, int32_t length) {
	if (dh_ == NULL) {
		FATAL("DHWrapper not initialized");
		return false;
	}

	if (shared_key_length_ != 0 || shared_key_ != NULL) {
		FATAL("Shared key already computed");
		return false;
	}

	shared_key_length_ = DH_size(dh_);
	if (shared_key_length_ <= 0 || shared_key_length_ > 1024) {
		FATAL("Unable to get shared key size in bytes");
		return false;
	}
	shared_key_ = new uint8_t[shared_key_length_];
	memset(shared_key_, 0, shared_key_length_);

	peer_public_key_ = BN_bin2bn(peer_public_key, length, 0);
	if (peer_public_key_ == NULL) {
		FATAL("Unable to get the peer public key");
		return false;
	}

	if (DH_compute_key(shared_key_, peer_public_key_, dh_) == -1) {
		FATAL("Unable to compute the shared key");
		return false;
	}

	return true;
}

bool DHWrapper::CopySharedKey(uint8_t *dst, int32_t dst_length) {
	if (dh_ == NULL) {
		FATAL("DHWrapper not initialized");
		return false;
	}

	if (dst_length != shared_key_length_) {
		FATAL("Destination has different size");
		return false;
	}

	memcpy(dst, shared_key_, shared_key_length_);

	return true;
}

void DHWrapper::Cleanup() {
	if (dh_ != NULL) {
		if (dh_->p != NULL) {
			BN_free(dh_->p);
			dh_->p = NULL;
		}
		if (dh_->g != NULL) {
			BN_free(dh_->g);
			dh_->g = NULL;
		}
		DH_free(dh_);
		dh_ = NULL;
	}

	if (shared_key_ != NULL) {
		delete[] shared_key_;
		shared_key_ = NULL;
	}
	shared_key_length_ = 0;

	if (peer_public_key_ != NULL) {
		BN_free(peer_public_key_);
		peer_public_key_ = NULL;
	}
}

bool DHWrapper::CopyKey(BIGNUM *num, uint8_t *dst, int32_t dst_length) {
	int32_t key_size = BN_num_bytes(num);
	if ((key_size <= 0) || (dst_length <= 0) || (key_size > dst_length)) {
		FATAL("CopyPublicKey failed due to either invalid DH state or invalid call");
		return false;
	}

	if (BN_bn2bin(num, dst) != key_size) {
		FATAL("Unable to copy key");
		return false;
	}

	return true;
}

void InitRC4Encryption(uint8_t *secret_key, uint8_t *pub_key_in, uint8_t *pub_key_out,
		RC4_KEY *rc4_key_in, RC4_KEY *rc4_key_out) {
	uint8_t digest[SHA256_DIGEST_LENGTH];
	unsigned int digestLen = 0;

	HMAC_CTX ctx;
	HMAC_CTX_init(&ctx);
	HMAC_Init_ex(&ctx, secret_key, 128, EVP_sha256(), 0);
	HMAC_Update(&ctx, pub_key_in, 128);
	HMAC_Final(&ctx, digest, &digestLen);
	HMAC_CTX_cleanup(&ctx);

	RC4_set_key(rc4_key_out, 16, digest);

	HMAC_CTX_init(&ctx);
	HMAC_Init_ex(&ctx, secret_key, 128, EVP_sha256(), 0);
	HMAC_Update(&ctx, pub_key_out, 128);
	HMAC_Final(&ctx, digest, &digestLen);
	HMAC_CTX_cleanup(&ctx);

	RC4_set_key(rc4_key_in, 16, digest);
}

std::string md5(const std::string &source, bool text_result) {
  return md5((uint8_t *)STR(source), source.length(), text_result);
}

std::string md5(uint8_t *source, uint32_t length, bool text_result) {
	EVP_MD_CTX *mdctx;
	unsigned char md_value[EVP_MAX_MD_SIZE];
	unsigned int md_len;

  mdctx = EVP_MD_CTX_create();
	EVP_DigestInit(mdctx, EVP_md5());
	EVP_DigestUpdate(mdctx, source, length);
	EVP_DigestFinal_ex(mdctx, md_value, &md_len);
  EVP_MD_CTX_destroy(mdctx);
	//EVP_MD_CTX_cleanup(&mdctx);

	if (text_result) {
    std::stringstream result;
		for (uint32_t i = 0; i < md_len; i++) {
      result << std::setfill('0') << std::setw(2) << std::hex << (int)md_value[i];
		}
		return result.str();
	} else {
		return std::string((char *) md_value, md_len);
	}
}

void sha1(const void *data, uint32_t data_length, uint8_t *result) {
  SHA_CTX c;
  SHA1_Init(&c);
  SHA1_Update(&c, data, data_length);
  SHA1_Final(result, &c);

  // length of output alway be SHA_DIGEST_LENGTH (20) on sha1
}

void sha256(const void *data, uint32_t data_length, uint8_t *result) { 
  SHA256_CTX c;
  SHA256_Init(&c);
  SHA256_Update(&c, data, data_length);
  SHA256_Final(result, &c);

  // length of output alway be SHA256_DIGEST_LENGTH (32) on sha256
}

void sha512(const void *data, uint32_t data_length, uint8_t *result) { 
  SHA512_CTX c;
  SHA512_Init(&c);
  SHA512_Update(&c, data, data_length);
  SHA512_Final(result, &c);

  // length of output alway be SHA512_DIGEST_LENGTH (64) on sha512
}

std::string b64(const std::string &source) {
	return b64((uint8_t *) STR(source), source.size());
}

std::string b64(uint8_t *buffer, uint32_t length) {
	BIO *bmem;
	BIO *b64;
	BUF_MEM *bptr;

	b64 = BIO_new(BIO_f_base64());
	bmem = BIO_new(BIO_s_mem());

	b64 = BIO_push(b64, bmem);
	BIO_write(b64, buffer, length);
  std::string result;
	if (BIO_flush(b64) == 1) {
		BIO_get_mem_ptr(b64, &bptr);
		result = std::string(bptr->data, bptr->length);
	}

	BIO_free_all(b64);


	replace(result, "\n", "");
	replace(result, "\r", "");

	return result;
}

std::string unb64(const std::string &source) {
	return unb64((uint8_t *) STR(source), source.length());
}

std::string unb64(uint8_t *buffer, uint32_t length) {
	// create a memory buffer containing base64 encoded data
	BIO* bmem = BIO_new_mem_buf((void *) buffer, length);

	// push a Base64 filter so that reading from buffer decodes it
	BIO *bioCmd = BIO_new(BIO_f_base64());
	// we don't want newlines
	BIO_set_flags(bioCmd, BIO_FLAGS_BASE64_NO_NL);
	bmem = BIO_push(bioCmd, bmem);

	char *pOut = new char[length];

	int finalLen = BIO_read(bmem, (void*) pOut, length);
	BIO_free_all(bmem);
  std::string result(pOut, finalLen);
	delete[] pOut;
	return result;
}

std::string unhex(std::string source) {
	if (source == "")
		return "";
	if ((source.length() % 2) != 0) {
		FATAL("Invalid hex string: %s", STR(source));
		return "";
	}
	source = lowerCase(source);
  std::string result;
	for (uint32_t i = 0; i < (source.length() / 2); i++) {
		uint8_t val = 0;
		if ((source[i * 2] >= '0') && (source[i * 2] <= '9')) {
			val = (source[i * 2] - '0') << 4;
		} else if ((source[i * 2] >= 'a') && (source[i * 2] <= 'f')) {
			val = (source[i * 2] - 'a' + 10) << 4;
		} else {
			FATAL("Invalid hex string: %s", STR(source));
			return "";
		}
		if ((source[i * 2 + 1] >= '0') && (source[i * 2 + 1] <= '9')) {
			val |= (source[i * 2 + 1] - '0');
		} else if ((source[i * 2 + 1] >= 'a') && (source[i * 2 + 1] <= 'f')) {
			val |= (source[i * 2 + 1] - 'a' + 10);
		} else {
			FATAL("Invalid hex string: %s", STR(source));
			return "";
		}
		result += (char) val;
	}
	return result;
}

void CleanupSSL() {
	ERR_remove_state(0);
	ENGINE_cleanup();
	CONF_modules_unload(1);
	ERR_free_strings();
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
}



}

