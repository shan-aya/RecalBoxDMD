// ============================================
// mini_inflate.h -- inflateur zlib/deflate minimal (~1,3 Ko de RAM de travail,
// PILE uniquement, aucune allocation heap).
//
// safe-modify -- Historique des modifications
// Version actuelle : v1
//
// v1 - 2026-09-19 - safe-modify - Creation. But : remplacer le decompresseur
// miniz/tinfl du mode Pinball (vpinball_dmd.h), dont l'etat de ~11 Ko devait
// etre alloue d'un seul bloc contigu -- impossible en regime etabli (heap
// fragmente : ~22 Ko libres mais maxalloc 4,6 Ko, mesure DMD2), d'ou le
// reboot dedie a chaque lancement de table. Ici tout tient sur la pile.
//
// Algorithme : derive directement de puff.c (Mark Adler, licence zlib), la
// reference "minimale et lisible" de zlib : decodage Huffman canonique par
// comptage (count[]/symbol[]), pas de table de recherche rapide -- plus lent
// que tinfl mais largement suffisant pour des trames de zones DMD de
// quelques centaines d'octets. La sortie sert de fenetre (pas de dictionnaire
// separe de 32 Ko) : la sortie doit donc tenir ENTIEREMENT dans le buffer
// fourni, comme le faisait deja le mode NON_WRAPPING de tinfl.
//
// Format d'entree : flux zlib (en-tete 2 octets + deflate + adler32 final,
// ce dernier ignore -- comme le faisait deja tinfl sans le flag adler32).
// ============================================
#pragma once
#include <stdint.h>
#include <stddef.h>

#define MINF_MAXBITS   15
#define MINF_MAXLCODES 286
#define MINF_MAXDCODES 30
#define MINF_FIXLCODES 288

struct minf_huff {
  int16_t count[MINF_MAXBITS + 1];
  int16_t symbol[MINF_FIXLCODES];
};

struct minf_state {
  uint8_t       *out;
  size_t         outlen, outcnt;
  const uint8_t *in;
  size_t         inlen, incnt;
  uint32_t       bitbuf;
  int            bitcnt;
  int            err;   // != 0 des qu'un octet d'entree manque
};

static int minf_bits(minf_state *s, int need)
{
  uint32_t val = s->bitbuf;
  while (s->bitcnt < need)
  {
    if (s->incnt >= s->inlen) { s->err = 1; return 0; }
    val |= (uint32_t)s->in[s->incnt++] << s->bitcnt;
    s->bitcnt += 8;
  }
  s->bitbuf = val >> need;
  s->bitcnt -= need;
  return (int)(val & ((1u << need) - 1));
}

static int minf_stored(minf_state *s)
{
  s->bitbuf = 0;
  s->bitcnt = 0;
  if (s->incnt + 4 > s->inlen) return 2;
  unsigned len = s->in[s->incnt++];
  len |= (unsigned)s->in[s->incnt++] << 8;
  const unsigned nlen = s->in[s->incnt] | ((unsigned)s->in[s->incnt + 1] << 8);
  s->incnt += 2;
  if (nlen != (~len & 0xffff)) return -2;
  if (s->incnt + len > s->inlen) return 2;
  if (s->outcnt + len > s->outlen) return 1;
  while (len--) s->out[s->outcnt++] = s->in[s->incnt++];
  return 0;
}

static int minf_decode(minf_state *s, const minf_huff *h)
{
  int code = 0, first = 0, index = 0;
  for (int len = 1; len <= MINF_MAXBITS; len++)
  {
    code |= minf_bits(s, 1);
    if (s->err) return -1;
    const int count = h->count[len];
    if (code - count < first) return h->symbol[index + (code - first)];
    index += count;
    first += count;
    first <<= 1;
    code <<= 1;
  }
  return -10;
}

static int minf_construct(minf_huff *h, const int16_t *length, int n)
{
  int16_t offs[MINF_MAXBITS + 1];
  for (int len = 0; len <= MINF_MAXBITS; len++) h->count[len] = 0;
  for (int symbol = 0; symbol < n; symbol++) h->count[length[symbol]]++;
  if (h->count[0] == n) return 0;
  int left = 1;
  for (int len = 1; len <= MINF_MAXBITS; len++)
  {
    left <<= 1;
    left -= h->count[len];
    if (left < 0) return left;
  }
  offs[1] = 0;
  for (int len = 1; len < MINF_MAXBITS; len++) offs[len + 1] = offs[len] + h->count[len];
  for (int symbol = 0; symbol < n; symbol++)
    if (length[symbol] != 0) h->symbol[offs[length[symbol]]++] = symbol;
  return left;
}

static int minf_codes(minf_state *s, const minf_huff *lencode, const minf_huff *distcode)
{
  static const int16_t lens[29] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
  static const int16_t lext[29] = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
  static const int16_t dists[30] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
  static const int16_t dext[30] = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
  int symbol;
  do
  {
    symbol = minf_decode(s, lencode);
    if (symbol < 0) return symbol;
    if (symbol < 256)
    {
      if (s->outcnt >= s->outlen) return 1;
      s->out[s->outcnt++] = (uint8_t)symbol;
    }
    else if (symbol > 256)
    {
      symbol -= 257;
      if (symbol >= 29) return -10;
      int len = lens[symbol] + minf_bits(s, lext[symbol]);
      symbol = minf_decode(s, distcode);
      if (symbol < 0) return symbol;
      const size_t dist = (size_t)dists[symbol] + minf_bits(s, dext[symbol]);
      if (s->err) return -1;
      if (dist > s->outcnt) return -11;
      if (s->outcnt + len > s->outlen) return 1;
      while (len--) { s->out[s->outcnt] = s->out[s->outcnt - dist]; s->outcnt++; }
    }
  } while (symbol != 256);
  return 0;
}

static int minf_fixed(minf_state *s, minf_huff *lencode, minf_huff *distcode)
{
  int16_t lengths[MINF_FIXLCODES];
  int symbol;
  for (symbol = 0; symbol < 144; symbol++) lengths[symbol] = 8;
  for (; symbol < 256; symbol++) lengths[symbol] = 9;
  for (; symbol < 280; symbol++) lengths[symbol] = 7;
  for (; symbol < MINF_FIXLCODES; symbol++) lengths[symbol] = 8;
  minf_construct(lencode, lengths, MINF_FIXLCODES);
  for (symbol = 0; symbol < MINF_MAXDCODES; symbol++) lengths[symbol] = 5;
  minf_construct(distcode, lengths, MINF_MAXDCODES);
  return minf_codes(s, lencode, distcode);
}

static int minf_dynamic(minf_state *s, minf_huff *lencode, minf_huff *distcode)
{
  static const int16_t order[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
  int16_t lengths[MINF_MAXLCODES + MINF_MAXDCODES];
  const int nlen  = minf_bits(s, 5) + 257;
  const int ndist = minf_bits(s, 5) + 1;
  const int ncode = minf_bits(s, 4) + 4;
  if (s->err) return -1;
  if (nlen > MINF_MAXLCODES || ndist > MINF_MAXDCODES) return -3;
  int index;
  for (index = 0; index < ncode; index++) lengths[order[index]] = (int16_t)minf_bits(s, 3);
  for (; index < 19; index++) lengths[order[index]] = 0;
  if (s->err) return -1;
  if (minf_construct(lencode, lengths, 19) != 0) return -4;
  index = 0;
  while (index < nlen + ndist)
  {
    int symbol = minf_decode(s, lencode);
    if (symbol < 0) return symbol;
    if (symbol < 16)
    {
      lengths[index++] = (int16_t)symbol;
    }
    else
    {
      int len = 0;
      if (symbol == 16)
      {
        if (index == 0) return -5;
        len = lengths[index - 1];
        symbol = 3 + minf_bits(s, 2);
      }
      else if (symbol == 17) symbol = 3 + minf_bits(s, 3);
      else                   symbol = 11 + minf_bits(s, 7);
      if (s->err) return -1;
      if (index + symbol > nlen + ndist) return -6;
      while (symbol--) lengths[index++] = (int16_t)len;
    }
  }
  if (lengths[256] == 0) return -9;
  int err = minf_construct(lencode, lengths, nlen);
  if (err && (err < 0 || nlen != lencode->count[0] + lencode->count[1])) return -7;
  err = minf_construct(distcode, lengths + nlen, ndist);
  if (err && (err < 0 || ndist != distcode->count[0] + distcode->count[1])) return -8;
  return minf_codes(s, lencode, distcode);
}

// Decompresse un flux zlib. *outLen : capacite de `out` en entree, octets
// produits en sortie. Retourne 0 si OK, un code d'erreur != 0 sinon (1 = sortie
// trop petite, 2 = entree tronquee, < 0 = flux invalide).
static int minf_zlib_inflate(uint8_t *out, size_t *outLen, const uint8_t *in, size_t inLen)
{
  if (inLen < 2) return 2;
  if ((in[0] & 0x0f) != 8 || (((unsigned)in[0] << 8) | in[1]) % 31 != 0 || (in[1] & 0x20)) return -1;
  minf_state s;
  s.out = out; s.outlen = *outLen; s.outcnt = 0;
  s.in = in; s.inlen = inLen; s.incnt = 2;
  s.bitbuf = 0; s.bitcnt = 0; s.err = 0;
  minf_huff lencode, distcode;
  int last, type, err = 0;
  do
  {
    last = minf_bits(&s, 1);
    type = minf_bits(&s, 2);
    if (s.err) { err = 2; break; }
    err = (type == 0) ? minf_stored(&s)
        : (type == 1) ? minf_fixed(&s, &lencode, &distcode)
        : (type == 2) ? minf_dynamic(&s, &lencode, &distcode)
        : -1;
    if (err != 0) break;
  } while (!last);
  if (err == -1 && s.err) err = 2;
  *outLen = s.outcnt;
  return err;
}
