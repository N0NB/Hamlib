# Current Autotools documentation suggests that DejaGNU is obsolete
# and replaced by Autotest.  TODO: implement Autotest

# AUTOMAKE_OPTIONS = dejagnu
# DEJATOOL = testfreq testbcd testloc rigctl

DISTCLEANFILES = 

bin_PROGRAMS = 

check_PROGRAMS = simelecraft simicgeneric simkenwood simyaesu simic9100 simic9700 simft991 simftdx1200 simftdx3000 simjupiter simpowersdr simid5100 simft736 simftdx5000 simtmd700 simrotorez simspid simft817 simts590 simft847 simic7300 simic7000 simic7100 simic7200 simatd578 simic905 simts450 simic7600 simic7610 simic705 simts950 simts990 simic7851 simftdx101 simxiegug90 simqrplabs simft818 simic275 simtrusdx simft1000 simtmd710 simts890 simxiegux108g simxiegux6100 simic910 simft450 simelecraftk4 simmicom simflex simft710 simic2730 simorion simpmr171 simic7700 simft990 simpstrotator simeasycomm simicr8600

# include generated include files ahead of any in sources
#rigctl_CPPFLAGS = -I$(top_builddir)/tests -I$(top_builddir)/src -I$(srcdir) $(AM_CPPFLAGS)

# all the programs need this
LDADD = $(top_builddir)/src/libhamlib.la $(top_builddir)/lib/libmisc.la $(DL_LIBS)

simelecraft_CFLAGS = $(AM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_builddir)/src
simkenwood_CFLAGS = $(AM_CFLAGS) $(PTHREAD_CFLAGS) -I$(top_builddir)/src
simyaesu_CFLAGS = $(AM_CFLAGS) $(PTHREAD_CFLAGS) -I$(top_builddir)/src
simid5100_CFLAGS = $(AM_CFLAGS) $(PTHREAD_CFLAGS) -I$(top_builddir)/src

simelecraft_LDADD = $(PTHREAD_LIBS) $(READLINE_LIBS) $(LDADD)
simkenwood_LDADD = $(PTHREAD_LIBS) $(LDADD) $(READLINE_LIBS)
simyaesu_LDADD = $(NET_LIBS) $(PTHREAD_LIBS) $(LDADD) $(READLINE_LIBS)
simid5100_LDADD = $(NET_LIBS) $(PTHREAD_LIBS) $(LDADD) $(READLINE_LIBS)

# Linker options
simelecraft_LDFLAGS = $(WINEXELDFLAGS)
simkenwood_LDFLAGS = $(WINEXELDFLAGS)
simyaesu_LDFLAGS = $(WINEXELDFLAGS)
simid5100_LDFLAGS = $(WINEXELDFLAGS)

EXTRA_DIST = sim.h

# Support 'make check' target for simple tests
#check_SCRIPTS = 

#TESTS = $(check_SCRIPTS)

$(top_builddir)/src/libhamlib.la:
	$(MAKE) -C $(top_builddir)/src/ libhamlib.la
