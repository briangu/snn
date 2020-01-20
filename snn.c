#include <assert.h>
#include <stdio.h>
#include <stdlib.h> 
#include <time.h>

#define MIN(a,b) (a < b ? a : b)

typedef unsigned char byte;

#define MAX_BYTE (0xFF)

#define IS_BIT_SET(b, pos) ((b >> pos) & 1)
#define IS_BIT_CLR(b, pos) (~(b >> pos) & 1)

#define SET_BIT(b, pos) (b |= (0x1 << pos))
#define CLR_BIT(b, pos) (b &= ~(0x1 << pos))

// the minimum membrane potential is 0 for all neurons and thus does not require memory storage. 
#define MIN_THRES ((byte)0)
#define LEAKAGE ((byte)1)
#define RAND_THRES_MARGIN ((byte)3)

typedef struct {
  // The output (1 = spike; 0 = no spike) of the 8 neurons is stored in byte OUTPS 
  // (1 bit per neuron) and that of the 8 sensors in byte INPS (1 bit per sensor)
  byte outps;
  byte inps;

  // The membrane potential of one neuron is stored in one byte of block MEMB,
  byte memb[8];
} State;

typedef struct {
  // The threshold is constant for all neurons and the stored in byte THRES;
  byte thres;

  // The sign of the 8 neurons is stored in byte SIGN (1 = positive; 0 = negative).
  byte sign;

  // The connectivity pattern of one neuron is stored in one byte of block NCONN (connections
  // from neurons) and in one byte of block ICONN (connections from sensors).
  byte nconn[8];
  byte iconn[8];
} Config;

#define ICONN(c, i) (c->iconn[i])
#define NCONN(c, i) (c->nconn[i])
#define SIGN(c, i) (c->sign)
#define THRES(c, i) (c->thres)

byte randOffset(byte thres, byte margin) {
  int k = (rand() % (2 * margin)) - margin;
  return (byte)(k < 0 ? thres - MIN(thres, k) : thres + MIN(MAX_BYTE - thres, k));
}

byte randBit(byte b) {
  int pos = rand() % 8;
  return (IS_BIT_SET(b, pos)) ? CLR_BIT(b, pos) : SET_BIT(b, pos);
}

void resetState(State *pState) {
  pState->inps = 0;
  pState->outps = 0;
  for (int i = 0; i < 8; i++) {
    pState->memb[i] = 0;
  }
}

void printState(State *pState) {
  printf("outps: %2x inps:%2x\n", pState->outps, pState->inps);
  for (int i = 0; i < 8; i++) {
    printf("memb[%d]: %2x ", i, pState->memb[i]);
  }
  printf("\n");
}

void resetConfig(Config *pConfig) {
  pConfig->thres = 0;
  pConfig->sign = 0;
  for (int i = 0; i < 8; i++) {
    pConfig->nconn[i] = 0;
    pConfig->iconn[i] = 0;
  }
}

void printConfig(Config *config) {
  for (int i = 0; i < 8; i++) {
    printf("thres: %2x sign:%2x iconn: %2x nconn: %2x\n", THRES(config, i), SIGN(config, i), ICONN(config, i), NCONN(config, i));
  }
}

byte popCount(byte b) {
  return __builtin_popcount(b);
//  byte count = 0;
//  while (b) {
//    count++;
//   b >>= 1;
//  }
//  return count;
}

void updateMembrane(State *pState, Config *pConfig, int i) {
  byte activeInps = popCount(pState->inps & ICONN(pConfig, i));
  byte activeOutps = pState->outps & NCONN(pConfig, i);
  byte posMemb = popCount(activeOutps & SIGN(pConfig, i));
  byte negMemb = popCount(activeOutps & ~SIGN(pConfig, i));
  pState->memb[i] += MIN(MAX_BYTE - pState->memb[i], activeInps + posMemb);
  pState->memb[i] -= MIN(pState->memb[i], negMemb);
}

void applyThreshold(State *pState, int i, byte thres, byte *tmp_outps) {
  if (pState->memb[i] >= thres) {
    SET_BIT(*tmp_outps, i);
    pState->memb[i] = 0;
  } else {
    CLR_BIT(*tmp_outps, i);
  }
}

void applyLeakage(State *pState, int i, byte leakage) {
  if (pState->memb[i] >= leakage) {
    pState->memb[i] -= leakage;
  }
}

void update(State *pState, Config *pConfig) {
  byte tmp_outps = 0;

  for (int i = 0; i < 8; i++) {
    // step 1 (if not just fired, update membrane)
    if (IS_BIT_CLR(pState->outps, i)) {
      // step 2
      updateMembrane(pState, pConfig, i);
    }

    // step 3
    applyThreshold(pState, i, randOffset(pConfig->thres, RAND_THRES_MARGIN), &tmp_outps);
    // step 4
    applyLeakage(pState, i, LEAKAGE);
  }

  pState->outps = tmp_outps;
}

void testUpdateMembrane() {
  Config config;
  State state;

  for (int i = 0; i < 8; i++) {
    resetState(&state);
    resetConfig(&config);
    updateMembrane(&state, &config, i);
    assert(state.memb[i] == 0);    

    resetState(&state);
    resetConfig(&config);
    config.iconn[i] = 0xFF;
    config.nconn[i] = 0xFF;
    config.sign = 0xAA;
    state.inps = 0;
    state.outps = 0xFF;

    updateMembrane(&state, &config, i);
    assert(state.memb[i] == 0);

    resetState(&state);
    resetConfig(&config);
    config.iconn[i] = 0xFF;
    config.nconn[i] = 0xFF;
    config.sign = 0xAA;
    state.inps = 0xFF;
    state.outps = 0x00;

    updateMembrane(&state, &config, i);
    assert(state.memb[i] == 8);

    resetState(&state);
    resetConfig(&config);
    config.iconn[i] = 0xFF;
    config.nconn[i] = 0xFF;
    config.sign = 0xAA;
    state.inps = 0xFF;
    state.outps = 0xFF;

    updateMembrane(&state, &config, i);
    assert(state.memb[i] == 8);
    updateMembrane(&state, &config, i);
    assert(state.memb[i] == 16);

    resetState(&state);
    resetConfig(&config);
    config.iconn[i] = 0xFF;
    config.nconn[i] = 0xFF;
    config.sign = 0xAA;
    state.inps = 0xFF;
    state.outps = (byte)~(0xAA);

    updateMembrane(&state, &config, i);
    assert(state.memb[i] == 4);
  }
}

void testApplyThreshold() {
  State state;

  resetState(&state);

  for (int i = 0; i < 8; i++) {
    byte tmp_outps = 0;
    applyThreshold(&state, i, 0, &tmp_outps);
    assert(IS_BIT_SET(tmp_outps, i));

    applyThreshold(&state, i, 10, &tmp_outps);
    assert(IS_BIT_CLR(tmp_outps, i));

    state.memb[i] = 10;
    applyThreshold(&state, i, 10, &tmp_outps);
    assert(IS_BIT_SET(tmp_outps, i));
  }
}

void testApplyLeakage() {
  State state;

  resetState(&state);

  for (int i = 0; i < 8; i++) {
    assert(state.memb[i] == 0);
    applyLeakage(&state, i, 1);
    assert(state.memb[i] == 0);

    state.memb[i] = 10;
    applyLeakage(&state, i, 1);
    assert(state.memb[i] == 9);
  }
}

void testUpdate() {
  testApplyThreshold();
  testApplyLeakage();
  testUpdateMembrane();
}

void testBitSet() {
  byte b = 0;

  for (int i = 0; i < 8; i++) {
    assert(!IS_BIT_SET(b, i));
    assert(IS_BIT_CLR(b, i));
    SET_BIT(b, i);
    assert(IS_BIT_SET(b, i));
    assert(!IS_BIT_CLR(b, i));
    CLR_BIT(b, i);
    assert(!IS_BIT_SET(b, i));
    assert(IS_BIT_CLR(b, i));
  }
}

void testPopCount() {
  byte b = 0;
  assert(popCount(b) == 0);

  for (int i = 0; i < 8; i++) {
    b = 0;
    SET_BIT(b, i);
    assert(popCount(b) == 1);
  }
}

void test() {
  testBitSet();
  testPopCount();
  testUpdate();
}


double evaluate(State *pState, Config *pConfig) {
  return popCount(pState->outps);
}

void evolveConfig(Config *pSource, Config *pChild) {
  int pos;
  *pChild = *pSource;
  switch (rand() % 4) {
    case 0:
      pChild->thres = randOffset(pChild->thres, 4);
      break;
    case 1:
      pChild->sign = randBit(pChild->sign);
      break;
    case 2: 
      pos = rand() % 8;
      pChild->iconn[pos] = randBit(pChild->iconn[pos]);
      break;
    case 3:
      pos = rand() % 8;
      pChild->nconn[pos] = randBit(pChild->iconn[pos]);
      break;
    default:
      break;
  }
}

void evolvePopulation(int kPopulationCount, Config population[kPopulationCount], int parentIdx) {
  for (int i = 0; i < kPopulationCount; i++) {
    if (i != parentIdx) {
      evolveConfig(&population[parentIdx], &population[i]);
    }
  }
}

void initRandomConfig(Config *pConfig) {
  resetConfig(pConfig);
  pConfig->thres = rand() % MAX_BYTE;
  pConfig->sign = rand() % MAX_BYTE;
  for (int i = 0; i < 8; i++) {
    pConfig->iconn[i] = rand() % MAX_BYTE;
    pConfig->nconn[i] = rand() % MAX_BYTE;
  }
}

Config runSimulation(int kPopulationCount, int kGenerations, int seed) {
  Config population[kPopulationCount];
  State state;

  srand(seed);

  for (int i = 0; i < kPopulationCount; i++) {
    initRandomConfig(&population[i]);
  }

  double bestScore = 0.0;
  int bestIndex = -1;

  for (int g = 0; g < kGenerations; g++) {
    bestScore = 0.0;
    bestIndex = -1;

    for (int i = 0; i < kPopulationCount; i++) {
      resetState(&state);
      state.inps = 0xFF;
      for (int t = 0; t < 100; t++) {
        update(&state, &population[i]);
        double score = evaluate(&state, &population[i]);
        if (score > bestScore) {
          bestScore = score;
          bestIndex = i;
        }
      }
      // printState(&state);
    }

    bestIndex = bestIndex < 0 ? 0 : bestIndex;

    evolvePopulation(kPopulationCount, population, bestIndex);
  }

  bestIndex = bestIndex < 0 ? 0 : bestIndex;

  printf("bestScore %f bestIndex: %d\n", bestScore, bestIndex);
  printConfig(&population[bestIndex]);

  return population[bestIndex];
}

int main(int argc, char** argv) {
  test();

  Config best = runSimulation(10, 10000, time(0));
  printConfig(&best);

  return 0;
}
