
MASTER_DIR=$(shell pwd)
BIN_DIR=$(MASTER_DIR)/bin

CFLAGS:=
ifeq ($(mode), debug)
	CFLAGS:=$(CFLAGS) -O0 -g -DDEBUG
else
	CFLAGS:=$(CFLAGS) -O3
endif

CXXFLAGS:=-std=c++11 $(CFLAGS)
export $(CFLAGS)
export $(CXXFLAGS)

COMPACT=$(BIN_DIR)/compact_vcf
COMPACT_FILTER=$(BIN_DIR)/compact_filter_vcf

all: $(COMPACT) $(COMPACT_FILTER)
.PHONY: all

$(COMPACT): compact_vcf.cpp
	@mkdir -p $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) -o $@ compact_vcf.cpp

$(COMPACT_FILTER): compact_filter_vcf.cpp
	@mkdir -p $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) -o $@ compact_filter_vcf.cpp

.PHONY: all

clean:
	@rm -rf $(BIN_DIR)
.PHONY: clean
