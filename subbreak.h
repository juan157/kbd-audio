/*! \file subbreak.h
 *  \brief Enter description here.
 *  \author Georgi Gerganov
 */

#pragma once

#include "constants.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

inline float frand() { return ((float)rand())/RAND_MAX; }

using TCode = int32_t;
using TProb = double;
using TGramLen = int;
using TFreqMap = std::tuple<TGramLen, std::vector<TProb>>;
using TAlphabet = std::vector<char>;

int kN = 27;
double pMin = -100;
std::array<int32_t, 256> myCharToInt = ::kCharToInt;

TCode calcCode(const char * data, int n) {
    TCode res = 0;
    do { res <<= 5; res += myCharToInt[*data++]; } while (--n > 0);
    return res;
}

template <typename T>
void shuffle(T & t, int start = -1, int end = -1) {
    if (start == -1) start = 0;
    if (end == -1) end = t.size();

    int n = t.size();
    for (int i = end - 1; i > start; --i) {
        std::swap(t[i], t[rand()%(i - start + 1)+start]);
    }
}

bool loadFreqMap(const char * fname, TFreqMap & res) {
    // auto & [len, fmap] = res;
    auto & len  = std::get<0>(res);
    auto & fmap = std::get<1>(res);

    len = 0;
    fmap.clear();

    printf("[+] Loading n-gram file '%s'\n", fname);
    std::ifstream fin(fname);

    if (fin.good() == false) {
        printf("    Failed to open file\n");
        return false;
    }

    std::string gram;
    int32_t nfreq = 0;
    int64_t nTotal = 0;

    while (true) {
        fin >> gram >> nfreq;
        if (fin.eof()) break;

        nTotal += nfreq;
    }

    fin.clear();
    fin.seekg(0);

    while (true) {
        fin >> gram >> nfreq;
        if (fin.eof()) break;

        if (len == 0) {
            len = gram.size();
            fmap.resize(1 << (5*len), 1.0);
        } else if (len != gram.size()) {
            printf("Error: loaded n-grams with vaying lengths\n");
            return false;
        }

        TCode idx = ::calcCode(gram.data(), len);
        if (fmap[idx] < 0.5) {
            printf("Error: duplicate n-gram '%s'\n", gram.c_str());
            return false;
        }
        fmap[idx] = std::log10(((double)(nfreq))/nTotal);
    }
    printf("    Total n-grams loaded = %g\n", (double) nTotal);

    //pMin = std::log(1000.0/nTotal);
    pMin = std::log(0.01/nTotal);
    printf("    P-min = %g\n", pMin);
    for (auto & p : fmap) {
        if (p >= 0.5) p = pMin;
    }

    return true;
}

TAlphabet getAlphabetRandom(int seed = 0) {
    TAlphabet res;

    for (int i = 0; i <= kN; ++i) res.push_back(i);
    shuffle(res, 1);

    return res;
}

bool encrypt(const std::string & plain, TAlphabet & alphabet, std::string & enc) {
    myCharToInt = kCharToInt;
    {
        int k = 26;
        for (auto & p : plain) {
            if (k == kN) break;
            if (myCharToInt[p] == 0) {
                myCharToInt[p] = ++k;
            }
        }
        kN = k;
        printf("[+] Unique symbols = %d\n", k);
    }

    alphabet = getAlphabetRandom(0);

    enc = plain;
    int n = plain.length();
    for (int i = 0; i < n; ++i) {
        auto c = ::myCharToInt[plain[i]];
        enc[i] = alphabet[c];
    }

    return true;
}

bool translate(const TAlphabet & alphabet, const std::string & src, std::string & dst) {
    if (dst.size() != src.size()) return false;

    for (int i = 0; i < (int) src.size(); ++i) {
        auto c = src[i];
        dst[i] = (c > 0 && c <= kN) ? alphabet[c] : '.';
    }

    return true;
}

TProb calcScore0(const TFreqMap & freqMap, const std::string & txt) {
    TProb res = 0.0;

    auto len = txt.size();
    //const auto & [n, fmap] = freqMap;
    const auto & n    = std::get<0>(freqMap);
    const auto & fmap = std::get<1>(freqMap);

    int i1 = 0;
    int cnt = 0;

    int k = n;
    TCode curc = 0;
    while (k > 0) {
        if (i1 >= len) return -1e100;
        auto c = txt[i1++];
        if (c > 0 && c <= 26) {
            curc <<= 5;
            curc += c;
            --k;
        } else {
            //res += pMin;
        }
    }

    TCode mask = (1 << 5*(n-1)) - 1;

    res += fmap[curc];
    ++cnt;
    while (true) {
        curc &= mask;

        while (true) {
            if (i1 >= len) return cnt > 0.3*len ? res/cnt : -1e100;
            auto c = txt[i1++];
            if (c > 0 && c <= 26) {
                curc <<= 5;
                curc += c;
                break;
            } else {
                //res += pMin;
            }
        }

        res += fmap[curc];
        ++cnt;
    }

    return cnt > 0.3*len ? res/cnt : -1e100;
}

TProb calcScore1(const TFreqMap & freqMap, const std::string & txt) {
    TProb res = 0.0;

    auto len = txt.size();
    //const auto & [n, fmap] = freqMap;
    const auto & n    = std::get<0>(freqMap);
    const auto & fmap = std::get<1>(freqMap);

    int i1 = 0;

    int k = n;
    TCode curc = 0;
    while (k > 0) {
        if (i1 >= len) return -1e100;
        auto c = txt[i1++];
        curc <<= 5;
        curc += c;
        --k;
    }

    TCode mask = (1 << 5*(n-1)) - 1;

    res += fmap[curc];
    while (true) {
        curc &= mask;

        if (i1 >= len) return res;
        auto c = txt[i1++];
        curc <<= 5;
        curc += c;

        res += fmap[curc];
    }

    return res;
}

auto calcScore = calcScore0;

void printText(const std::string & t) {
    for (auto & c : t) {
        if (c >= 1 && c <= 26) printf("%c", 'a' + c - 1); else printf(".");
    }
    printf("\n");
}

bool decrypt(const TFreqMap & freqMap, const std::string & enc, std::string & res, int nMainIters = 1e9) {
    TAlphabet besta = getAlphabetRandom();

    auto lena = kN;

    auto cure = enc;
    auto beste = enc;
    translate(besta, enc, cure);
    auto bestp = calcScore(freqMap, cure);
    printf("XXX %g\n", bestp);

    auto bestbestp = bestp;

    TProb pmax = 10.0;
    std::vector<TProb> pletter(kN + 1);
    std::fill(pletter.begin(), pletter.end(), pmax);

    int nIters = 0;
    while (nMainIters--) {
        if (++nIters > 10000) {
            TAlphabet besta = getAlphabetRandom();
            translate(besta, enc, cure);
            bestp = calcScore(freqMap, cure);
            printf("reset\n");
            nIters = 0;
        }

        auto itera = besta;
		int nswaps = 3;
        for (int i = 0; i < nswaps; ++i) {
            int a0 = rand()%lena + 1;
            int a1 = rand()%lena + 1;
            while (a0 == a1 || a0 > 26 || a1 <= 26) {
                a0 = rand()%lena + 1;
                a1 = rand()%lena + 1;
            }

            std::swap(itera[a0], itera[a1]);
        }

        translate(itera, enc, cure);
        auto iterp = calcScore(freqMap, cure);
        auto cura = itera;
        for (int i = 0; i < 100; ++i) {
            int a0 = rand()%26 + 1;
            int a1 = rand()%26 + 1;
            while (a0 == a1) {
                a0 = rand()%26 + 1;
                a1 = rand()%26 + 1;
            }

            std::swap(cura[a0], cura[a1]);

            translate(cura, enc, cure);
            auto curp = calcScore(freqMap, cure);
            if (curp > iterp) {
                iterp = curp;
                itera = cura;
                i = 0;
            } else {
                std::swap(cura[a0], cura[a1]);
            }
        }

        if (iterp > bestp) {
            besta = itera;
            bestp = iterp;

            if (bestp > bestbestp) {
                bestbestp = bestp;
                translate(besta, enc, beste);
                printf("[+] Best score = %g\n", bestp);
                printf("    Alphabet:  '");
                for (int i = 'a'; i <= 'z'; ++i) {
                    auto c = ::myCharToInt[i];
                    printf("%c", i - c + besta[c]);
                }
                printf("'\n");
                printText(beste);
                printf("[+] pmax = %g\n", pmax);
                printf("\n");
            }
            nIters = 0;
        }
    }

    return true;
}

