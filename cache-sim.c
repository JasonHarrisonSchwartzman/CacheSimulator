#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	unsigned int tag;
	unsigned short data;
	unsigned char valid;
} Cache;

Cache directMap1KB[32];
Cache directMap4KB[128];
Cache directMap16KB[512];
Cache directMap32KB[1024];

Cache setAssociative2[512]; 
Cache setAssociative4[512];
Cache setAssociative8[512]; 
Cache setAssociative16[512]; 
Cache *setAssoc[4] = {setAssociative2, setAssociative4, setAssociative8, setAssociative16};

Cache setAssociative2WM[512];
Cache setAssociative4WM[512];
Cache setAssociative8WM[512];
Cache setAssociative16WM[512];
Cache *setAssocWM[4] = {setAssociative2WM, setAssociative4WM, setAssociative8WM, setAssociative16WM};

Cache setAssociative2PF[512];
Cache setAssociative4PF[512];
Cache setAssociative8PF[512];
Cache setAssociative16PF[512];
Cache *setAssocPF[4] = {setAssociative2PF, setAssociative4PF, setAssociative8PF, setAssociative16PF};

Cache setAssociative2PFM[512];
Cache setAssociative4PFM[512];
Cache setAssociative8PFM[512];
Cache setAssociative16PFM[512];
Cache *setAssocPFM[4] = { setAssociative2PFM, setAssociative4PFM, setAssociative8PFM, setAssociative16PFM };

Cache fullAssociative[512];
Cache fullAssociativeHC[512];
int HCBits[512];

Cache *directMaps[4] = {directMap1KB, directMap4KB, directMap16KB, directMap32KB };
int sizes[4] = { 32, 128, 512, 1024 };
int numBits[4] = { 5, 7, 9, 10 };
int numHits[22]; //32, 128, 512, 1024 | 2, 4, 8, 16 | full, full hold cold | WM 2,4,8,16 | 2,4,8,16 PF | 
int numLines;

void directMapSim(unsigned int addr) {
	for (int i = 0; i < 4; i++) {
		unsigned int index = (addr >> 5) & (sizes[i] - 1);
		unsigned int tag = addr & (unsigned int)(0xFFFFFFFF << (5 + numBits[i]));
		if (!directMaps[i][index].valid) {
			directMaps[i][index].tag = tag;
			directMaps[i][index].valid = 1;
		}
		else {
			if ((directMaps[i][index].tag) == tag) numHits[i]++;		
			else directMaps[i][index].tag = tag;
		}
	}
}

int leafNodeToIndex(int leafIndex, int HC) {
	return 2*leafIndex - 512 + HC;
}
int indexToLeafNode(int index) {
	return (index + 512) / 2;
}

void fullAssociativeMapHCSim(unsigned int addr) {
	unsigned int tag = addr >> 5;
	for (int i = 0; i < 512; i++) {
		if (fullAssociativeHC[i].valid && (fullAssociativeHC[i].tag == tag)) {
			numHits[9]++;
			int leafNode = indexToLeafNode(i);
			HCBits[leafNode] = i % 2;
			for (int x = 0; x < 8; x++) {
				HCBits[leafNode/2] = leafNode % 2;
				leafNode/=2;
			}
			return;
		}
	}
	//miss
	int leafIndex = 1;
	for (int x = 0; x < 8; x++) {
		HCBits[leafIndex]^=1;
		leafIndex = leafIndex*2 + HCBits[leafIndex];
	}
	HCBits[leafIndex]^=1;
	int cacheIndex = leafNodeToIndex(leafIndex,HCBits[leafIndex]);
	fullAssociativeHC[cacheIndex].tag = tag;
	fullAssociativeHC[cacheIndex].valid = 1;
}

void fullAssociativeMapSim(unsigned int addr) {
	unsigned int tag = addr >> 5;
	for (int lineNum = 0; lineNum < 512; lineNum++) {
		if (fullAssociative[lineNum].tag == tag && fullAssociative[lineNum].valid) {
			numHits[8]++;
			int i = lineNum;
			while (i < 511) {
				if (!fullAssociative[i+1].valid) {
					break;
				}
				fullAssociative[i].tag = fullAssociative[i+1].tag;
				i++;
			}
			fullAssociative[i].tag = tag;
			return;
		}
		if (fullAssociative[lineNum].valid == 0) {
			fullAssociative[lineNum].valid = 1;
			fullAssociative[lineNum].tag = tag;
			return;
		}
	}
	for (int i = 0; i < 511; i++) {
		fullAssociative[i].tag = fullAssociative[i+1].tag;
	}
	fullAssociative[511].tag = tag;
}
void setAssociativeMapWMSim(unsigned int addr, char store) {
	unsigned int sets = 256;
	unsigned int lines = 2;
	unsigned int hitsIndex = 10;
	for (int i = 0; i < 4; i++) {
		unsigned int index = (addr >> 5) & (sets - 1);
		unsigned int tag = (addr >> (5 + 8 - (hitsIndex - 10)));
		int goNext = 0;
		for (int lineNum = 0; lineNum < lines; lineNum++) {
			if (setAssocWM[i][lines*index+lineNum].valid && (setAssocWM[i][lines*index+lineNum].tag == tag)) {
				numHits[hitsIndex]++;
				int x = lineNum;
				while (x < lines-1) {
					if (!setAssocWM[i][lines*index+x+1].valid) {
						break;
					}
					setAssocWM[i][lines*index+x].tag = setAssocWM[i][lines*index+x+1].tag;
					x++;
				}
				setAssocWM[i][lines*index+x].tag = tag;
				goNext = 1;
				break;
			}
			if (!setAssocWM[i][lines*index+lineNum].valid) {
				if (store != 'S') {
					setAssocWM[i][lines*index+lineNum].valid = 1;
					setAssocWM[i][lines*index+lineNum].tag = tag;
				}
				goNext = 1;
				break;
			}
		}
		if (!goNext && store != 'S') {
			for (int lineNum = 0; lineNum < lines - 1; lineNum++) {
				setAssocWM[i][lines*index+lineNum].tag = setAssocWM[i][lines*index+lineNum+1].tag;
			}
			setAssocWM[i][lines*index+lines-1].tag = tag;
		}
		sets >>= 1;
		lines <<=1;
		hitsIndex++;
	}

}
void setAssociativeMapSim(unsigned int addr) {
	unsigned int sets = 256;
	unsigned int lines = 2;
	unsigned int hitsIndex = 4;
	for (int i = 0; i < 4; i++) {
		unsigned int index = (addr >> 5) & (sets - 1);
		unsigned int tag = (addr >> (5 + 8 - (hitsIndex -4)));
		int goNext = 0;
		for (int lineNum = 0; lineNum < lines; lineNum++) {
			if (setAssoc[i][lines*index+lineNum].valid && (setAssoc[i][lines*index+lineNum].tag == tag)) {
				numHits[hitsIndex]++;
				int x = lineNum;
				while (x < lines-1) {
					if (!setAssoc[i][lines*index+x+1].valid) {
						break;
					}
					setAssoc[i][lines*index+x].tag = setAssoc[i][lines*index+x+1].tag;
					x++;
				}
				setAssoc[i][lines*index+x].tag = tag;
				goNext = 1;
				break;
			}
			if (!setAssoc[i][lines*index+lineNum].valid) {
				setAssoc[i][lines*index+lineNum].valid = 1;
				setAssoc[i][lines*index+lineNum].tag = tag;
				goNext = 1;
				break;
			}
		}
		if (!goNext) {
			for (int lineNum = 0; lineNum < lines - 1; lineNum++) {
				setAssoc[i][lines*index+lineNum].tag = setAssoc[i][lines*index+lineNum+1].tag;
			}	
			setAssoc[i][lines*index+lines-1].tag = tag;
		}
		sets >>= 1;
		lines <<= 1;
		hitsIndex++;
	}
}

void setAssociativeMapPFSim(unsigned int addr) {
	unsigned int address2 = addr + 32;
	unsigned int sets = 256;
	unsigned int lines = 2;
	unsigned int hitsIndex = 14;
	for (int i = 0; i < 4; i++) {
		unsigned int index = (addr >> 5) & (sets - 1);
		unsigned int tag = (addr >> (5 + 8 - (hitsIndex - 14)));
		unsigned int index2 = (address2 >> 5) & (sets - 1);
		unsigned int tag2 = (address2 >> (5 + 8 - (hitsIndex - 14)));
		int goNext = 0;
		int goNext2 = 0;
		for (int lineNum = 0; lineNum < lines; lineNum++) {
			if (setAssocPF[i][lines*index+lineNum].valid && (setAssocPF[i][lines*index+lineNum].tag == tag)) {
				numHits[hitsIndex]++;
				int x = lineNum;
				while (x < lines-1){
					if (!setAssocPF[i][lines*index+x+1].valid) {
						break;
					}
					setAssocPF[i][lines*index+x].tag = setAssocPF[i][lines*index+x+1].tag;
					x++;
				}
				setAssocPF[i][lines*index+x].tag = tag;
				goNext = 1;
				break;
			}
			if (!setAssocPF[i][lines*index+lineNum].valid) {
				setAssocPF[i][lines*index+lineNum].valid = 1;
				setAssocPF[i][lines*index+lineNum].tag = tag;
				goNext = 1;
				break;
			}
		}
		if (!goNext) {
			for (int lineNum = 0; lineNum < lines - 1; lineNum++) {
				setAssocPF[i][lines*index+lineNum].tag = setAssocPF[i][lines*index+lineNum+1].tag;
			}
			setAssocPF[i][lines*index+lines-1].tag = tag;
		}
		//addr2
		for (int lineNum = 0; lineNum < lines; lineNum++) {
			if (setAssocPF[i][lines*index2+lineNum].valid && (setAssocPF[i][lines*index2+lineNum].tag == tag2)){
				int x = lineNum;
				while (x < lines-1) {
					if (!setAssocPF[i][lines*index2+x+1].valid) {
						break;
					}
					setAssocPF[i][lines*index2+x].tag = setAssocPF[i][lines*index2+x+1].tag;
					x++;
				}
				setAssocPF[i][lines*index2+x].tag = tag2;
				goNext2 = 1;
				break;
			}
			if (!setAssocPF[i][lines*index2+lineNum].valid) {
				setAssocPF[i][lines*index2+lineNum].valid = 1;
				setAssocPF[i][lines*index2+lineNum].tag = tag2;
				goNext2 = 1;
				break;
			}
		}
		//addr2
		if (!goNext2) {
			for (int lineNum = 0; lineNum < lines - 1; lineNum++) {
				setAssocPF[i][lines*index2+lineNum].tag = setAssocPF[i][lines*index2+lineNum+1].tag;
			}
			setAssocPF[i][lines*index2+lines-1].tag = tag2;
		}
		sets >>=  1;
		lines <<= 1;
		hitsIndex++;
	}
}

void setAssociativeMapPFMSim(unsigned int addr) {
	unsigned int address2 = addr + 32;
	unsigned int sets = 256;
	unsigned int lines = 2;
	unsigned int hitsIndex = 18;
	for (int i = 0; i < 4; i++) {
		unsigned int index = (addr >> 5) & (sets - 1);
		unsigned int tag = (addr >> (5 + 8 - (hitsIndex - 18)));
		unsigned int index2 = (address2 >> 5) & (sets - 1);
		unsigned int tag2 = (address2 >> (5 + 8 - (hitsIndex - 18)));
		int goNext = 0;
		int goNext2 = 0;
		int hit = 0;
		for (int lineNum = 0; lineNum < lines; lineNum++) {
			if (setAssocPFM[i][lines*index+lineNum].valid && (setAssocPFM[i][lines*index+lineNum].tag == tag)) {
				numHits[hitsIndex]++;
				hit = 1;
				int x = lineNum;
				while (x < lines-1) {
					if (!setAssocPFM[i][lines*index+x+1].valid) {
						break;
					}
					setAssocPFM[i][lines*index+x].tag = setAssocPFM[i][lines*index+x+1].tag;
					x++;
				}
				setAssocPFM[i][lines*index+x].tag = tag;
				goNext = 1;
				break;
			}
			if (!setAssocPFM[i][lines*index+lineNum].valid) {
				setAssocPFM[i][lines*index+lineNum].valid = 1;
				setAssocPFM[i][lines*index+lineNum].tag = tag;
				goNext = 1;
				break;
			}
		}
		if (!goNext) {
			for (int lineNum = 0; lineNum < lines - 1; lineNum++) {
				setAssocPFM[i][lines*index+lineNum].tag = setAssocPFM[i][lines*index+lineNum+1].tag;
			}
			setAssocPFM[i][lines*index+lines-1].tag = tag;
		}
		if (!hit) {
			for (int lineNum = 0; lineNum < lines; lineNum++) {
				if (setAssocPFM[i][lines*index2+lineNum].valid && (setAssocPFM[i][lines*index2+lineNum].tag == tag2)) {
					int x = lineNum;
					while (x < lines-1) {
						if (!setAssocPFM[i][lines*index2+x+1].valid) {
							break;
						}
						setAssocPFM[i][lines*index2+x].tag = setAssocPFM[i][lines*index2+x+1].tag;
						x++;
					}
					setAssocPFM[i][lines*index2+x].tag = tag2;
					goNext2 = 1;
					break;
				}
				if (!setAssocPFM[i][lines*index2+lineNum].valid) {
					setAssocPFM[i][lines*index2+lineNum].valid = 1;
					setAssocPFM[i][lines*index2+lineNum].tag = tag2;
					goNext2 = 1;
					break;
				}
			}
			if (!goNext2) {
				for (int lineNum = 0; lineNum < lines - 1; lineNum++) {
					setAssocPFM[i][lines*index2+lineNum].tag = setAssocPFM[i][lines*index2+lineNum+1].tag;
				}
				setAssocPFM[i][lines*index2+lines-1].tag = tag2;
			}
		}
		sets >>= 1;
		lines <<= 1;
		hitsIndex++;
	}
}



int main(int argc, char*argv[]) {
	if (argc < 2) {
		printf("Not enough args!");
		return 1;
	}
	FILE *input;
	FILE *output;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	input = fopen(argv[1],"r");
	output = fopen(argv[2],"w");
	while ((read = getline(&line, &len, input)) != -1) {
		numLines++;
		short i = 0;
		char *traceLine[2];
		char *p = strtok(line, " ");
		while (p != NULL) {
			traceLine[i++] = p;
			p = strtok(NULL, " ");
		}
		unsigned int address = (unsigned)(int)strtol(traceLine[1],NULL,0);
		directMapSim(address);
		setAssociativeMapSim(address);
		fullAssociativeMapSim(address);
		setAssociativeMapWMSim(address,*traceLine[0]);
		setAssociativeMapPFSim(address);
		setAssociativeMapPFMSim(address);
		fullAssociativeMapHCSim(address);
	}

	//output
	fprintf(output,"%d,%d; %d,%d; %d,%d; %d,%d;\n",numHits[0],numLines,numHits[1],numLines,numHits[2],numLines,numHits[3],numLines);
	fprintf(output,"%d,%d; %d,%d; %d,%d; %d,%d;\n",numHits[4],numLines,numHits[5],numLines,numHits[6],numLines,numHits[7],numLines);
	fprintf(output,"%d,%d;\n",numHits[8],numLines);
	fprintf(output,"%d,%d;\n",numHits[9],numLines);
	fprintf(output,"%d,%d; %d,%d; %d,%d; %d,%d;\n",numHits[10],numLines,numHits[11],numLines,numHits[12],numLines,numHits[13],numLines);
	fprintf(output,"%d,%d; %d,%d; %d,%d; %d,%d;\n",numHits[14],numLines,numHits[15],numLines,numHits[16],numLines,numHits[17],numLines);
	fprintf(output,"%d,%d; %d,%d; %d,%d; %d,%d;\n",numHits[18],numLines,numHits[19],numLines,numHits[20],numLines,numHits[21],numLines);
	fclose(input);
	fclose(output);
	return 0;
}


