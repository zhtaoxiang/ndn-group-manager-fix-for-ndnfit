/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California
 *
 * This file is part of gep (Group-based Encryption Protocol for NDN).
 * See AUTHORS.md for complete list of gep authors and contributors.
 *
 * gep is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * gep is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * gep, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ndn-cxx/encoding/buffer-stream.hpp>
#include "rsa.hpp"
#include "error.hpp"

namespace ndn {
namespace gep {
namespace algo {

using namespace CryptoPP;

static Buffer
transform(SimpleProxyFilter* filter, const uint8_t* data, size_t dataLen)
{
  OBufferStream obuf;
  filter->Attach(new FileSink(obuf));

  StringSource pipe(data, dataLen, true, filter);
  return *(obuf.buf());
}

DecryptKey<Rsa>
Rsa::generateKey(RandomNumberGenerator& rng, RsaKeyParams& params)
{
  RSA::PrivateKey privateKey;
  privateKey.GenerateRandomWithKeySize(rng, params.getKeySize());

  OBufferStream obuf;
  privateKey.Save(FileSink(obuf).Ref());

  DecryptKey<Rsa> decryptKey(std::move(*obuf.buf()));
  return decryptKey;
}

EncryptKey<Rsa>
Rsa::deriveEncryptKey(const Buffer& keyBits)
{
  RSA::PrivateKey privateKey;

  ByteQueue keyQueue;
  keyQueue.LazyPut(keyBits.get(), keyBits.size());
  privateKey.Load(keyQueue);

  RSA::PublicKey publicKey(privateKey);

  OBufferStream obuf;
  publicKey.Save(FileSink(obuf).Ref());

  EncryptKey<Rsa> encryptKey(std::move(*obuf.buf()));
  return encryptKey;
}

Buffer
Rsa::decrypt(const uint8_t* key, size_t keyLen,
             const uint8_t* payload, size_t payloadLen,
             const EncryptParams& params)
{
  AutoSeededRandomPool rng;
  RSA::PrivateKey privateKey;

  ByteQueue keyQueue;
  keyQueue.LazyPut(key, keyLen);
  privateKey.Load(keyQueue);

  switch (params.getAlgorithmType()) {
    case tlv::AlgorithmRsaPkcs: {
      RSAES_PKCS1v15_Decryptor decryptor_pkcs1v15(privateKey);
      PK_DecryptorFilter* filter_pkcs1v15 = new PK_DecryptorFilter(rng, decryptor_pkcs1v15);
      return transform(filter_pkcs1v15, payload, payloadLen);
    }
    case tlv::AlgorithmRsaOaep: {
      RSAES_OAEP_SHA_Decryptor decryptor_oaep_sha(privateKey);
      PK_DecryptorFilter* filter_oaep_sha = new PK_DecryptorFilter(rng, decryptor_oaep_sha);
      return transform(filter_oaep_sha, payload, payloadLen);
    }
    default:
      throw Error("unsupported padding scheme");
  }
}

Buffer
Rsa::encrypt(const uint8_t* key, size_t keyLen,
             const uint8_t* payload, size_t payloadLen,
             const EncryptParams& params)
{
  AutoSeededRandomPool rng;
  RSA::PublicKey publicKey;

  ByteQueue keyQueue;
  keyQueue.LazyPut(key, keyLen);
  publicKey.Load(keyQueue);

  switch (params.getAlgorithmType()) {
    case tlv::AlgorithmRsaPkcs: {
      RSAES_PKCS1v15_Encryptor encryptor_pkcs1v15(publicKey);
      PK_EncryptorFilter* filter_pkcs1v15 = new PK_EncryptorFilter(rng, encryptor_pkcs1v15);
      return transform(filter_pkcs1v15, payload, payloadLen);
    }
    case tlv::AlgorithmRsaOaep: {
      RSAES_OAEP_SHA_Encryptor encryptor_oaep_sha(publicKey);
      PK_EncryptorFilter* filter_oaep_sha = new PK_EncryptorFilter(rng, encryptor_oaep_sha);
      return transform(filter_oaep_sha, payload, payloadLen);
    }
    default:
      throw Error("unsupported padding scheme");
  }
}

} // namespace algo
} // namespace gep
} // namespace ndn
