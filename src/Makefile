 
# sca 1 - piece of scake
# simple aes sca challenge without any delays
# define DRANDOM_DELAY to enable random delays
# define DTRIGGER=1 to enable a trigger right before entering aes_enc()
simplesca: CHNAME=simplesca
simplesca: OBJECTS=$(addprefix bin/,$(SERIAL) $(AESPROTECTED) $(SIMPLESCA)) aes/gf256mul.S 
simplesca: ADDFLAGS=-Iserial -Iaesprotected -DDELAYSIZE=0x0 -DTRIGGER=1 -DENABLE_RANDOMDELAY=0 -DENABLE_DUMMYROUNDS=0 -DENABLE_ANTIDFA=0 -DAES_ENCRYPTION=0
simplesca: serial aesprotected1 $(addprefix simplesca/,$(SIMPLESCA))
simplesca: simplesca.enc.hex

# sca 2 - still not scary
mediumsca: CHNAME=mediumsca
mediumsca: OBJECTS=$(addprefix bin/,$(SERIAL) $(AESPROTECTED) $(MEDIUMSCA)) aes/gf256mul.S 
mediumsca: ADDFLAGS=-Iserial -Iaesprotected -DDELAYSIZE=0x0 -DTRIGGER=0 -DENABLE_RANDOMDELAY=1
mediumsca: serial aesprotected2 $(addprefix mediumsca/,$(MEDIUMSCA))
mediumsca: mediumsca.enc.hex

# sca 3 - wow that escalated quickly
hardsca: CHNAME=hardsca
hardsca: OBJECTS=$(addprefix bin/,$(SERIAL) $(AESPROTECTED) $(HARDSCA)) aes/gf256mul.S 
hardsca: ADDFLAGS=-Iserial -Iaesprotected -DDELAYSIZE=0x0 -DTRIGGER=0 -DENABLE_RANDOMDELAY=1 -DENABLE_DUMMYROUNDS=1 -DENABLE_ANTIDFA=1 -DAES_ENCRYPTION=1
hardsca: serial aesprotected3 $(addprefix hardsca/,$(HARDSCA))
hardsca: hardsca.enc.hex
