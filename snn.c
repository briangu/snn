
#define MIN(a,b) (a < b ? a : b)

typedef char byte;

#define MAX_BYTE (0xFF)

#define IS_BIT_SET(b, pos) ((b >> pos) & 1)
#define IS_BIT_CLR(b, pos) (~(b >> pos) & 1)

#define SET_BIT(b, pos) (b |= (0x1 << pos))
#define CLR_BIT(b, pos) (b &= ~(0x1 << pos))

// the minimum membrane potential is 0 for all neurons and thus does not require memory storage. 
#define MIN_THRES ((byte)0)
#define LEAKAGE ((byte)1)

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

#define RAND_THRES(ps) ((byte)1)

#define ICONN(c, i) (c->iconn[i])
#define NCONN(c, i) (c->nconn[i])
#define SIGN(c, i) (c->sign)
#define THRES(c, i) (c->thres)

void resetState(State *pState) {
  pState->inps = 0;
  pState->outps = 0;
  for (int i = 0; i < 8; i++) {
    pState->memb[i] = 0;
  }
}

void resetConfig(Config *pConfig) {
  pConfig->thres = 0;
  pConfig->sign = 0;
  for (int i = 0; i < 8; i++) {
    pConfig->nconn[i] = 0;
    pConfig->iconn[i] = 0;
  }
}

byte popCount(byte b) {
  byte count = 0;
  while (b) {
    count++;
    b >>= 1;
  }
  return count;
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
    applyThreshold(pState, i, RAND_THRES(pConfig->thres), &tmp_outps);
    // step 4
    applyLeakage(pState, i, LEAKAGE);
  }

  pState->outps = tmp_outps;
}

int main(int argc, char** argv) {
  return 0;
}