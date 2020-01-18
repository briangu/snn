

typedef char byte;

#define IS_BIT_SET(b, pos) ((b >> pos) & 1)
#define SET_BIT(b, pos) ((b >> pos) & 1)
#define CLR_BIT(b, pos) ((b >> pos) & 1)

#define RAND_THRES(ps) ((byte)1)

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

void reset() {
  thres = 0;
  inps = 0;
  outps = 0;
  sign = 0;
  for (int i = 0; i < 8; i++) {
    memb[i] = 0;
    nconn[i] = 0;
    iconn[i] = 0;
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

void update(State* pState) {
  // for each neuron
  for (int i = 0; i < 8; i++) {
    // step 1
    if (!IN_REFACTORY_PERIOD(pState, )) {
      // step 2
      for (int j = 0; j < 8; j++) {
        byte activeInps = popCount(inps & pState->iconn[i]);
        byte activeOutps = pstate->outps & pState->nconn[i];
        byte posMemb = popCount(activeOutps & pState->sign);
        byte negMemb = popCount(activeOutps & ~(pState->sign));
        pState->memb[i] += activeInps + posMemb;
        pState->memb[i] = negMemb >= pState->memb[i] ? 0 : pState->memb[i] - negMemb;
      }
    }
    // step 3
    if (pState->memb[i] >= RAND_THRES(pState->thres)) {
      SET_BIT(pState->outps, i);
      pState->memb[i] = 0;
    } else {
      CLR_BIT(pState->outps, i);
    }
    // step 4
    if (pState->memb[i] >= LEAKAGE) {
      pState->memb[i] -= LEAKAGE;
    }
  }
}

int main(int argc, char** argv) {
  return 0;
}