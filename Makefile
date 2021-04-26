# Upgraded to c++17 to support the Filesystem library
CXXFLAGS     = -std=c++17 -fopenmp -O3 -D_GLIBCXX_PARALLEL
OBJDIR       = obj
DEPDIR       = $(OBJDIR)/.deps
# Flags which, when added to gcc/g++, will auto-generate dependency files
DEPFLAGS     = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

# Function which takes a list of words and returns a list of unique words in that list
# https://stackoverflow.com/questions/16144115/makefile-remove-duplicate-words-without-sorting
uniq         = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))

# Source files - add more to auto-compile into .o files
SOURCES      = Common/image.cpp ExperimentA/main.cpp
INCLUDES     = -I include/
# Executable targets - add more to auto-make in default 'all' target
EXEC         = ExperimentA/experiment
# Targets required for the homework, spearated by part
REQUIRED_1   = out/mean-H.pgm out/eigenface-H-largest-1.pgm out/eigenface-L-largest-1.pgm
REQUIRED_2   = 
REQUIRED_OUT = $(REQUIRED_1) $(REQUIRED_2)

SOURCEDIRS   = $(call uniq, $(dir $(SOURCES)))
OBJDIRS      = $(addprefix $(OBJDIR)/, $(SOURCEDIRS))
DEPDIRS      = $(addprefix $(DEPDIR)/, $(SOURCEDIRS))
DEPFILES     = $(SOURCES:%.cpp=$(DEPDIR)/%.d)

.PHONY: all exec clean report required
.SECONDARY:

# By default, make all executable targets and the outputs required for the homework
all: exec $(REQUIRED_OUT) Report/report.pdf
required: $(REQUIRED_OUT)
exec: $(EXEC)

# Executable Targets
ExperimentA/experiment: $(OBJDIR)/ExperimentA/main.o $(OBJDIR)/Common/image.o
	$(CXX) $(CXXFLAGS) $^ -o $@

### Experiment Outputs ###
out/training-%.dat: ExperimentA/experiment | out
	ExperimentA/experiment train Images/fa_$* $@

out/training-intruder-%.dat: ExperimentA/experiment | out
	ExperimentA/experiment train Images/fa2_$* $@

out/mean-%.pgm: ExperimentA/experiment out/training-%.dat
	ExperimentA/experiment info out/training-$*.dat -m $@

out/eigenface-%-largest-1.pgm out/eigenface-%-largest-2.pgm out/eigenface-%-largest-3.pgm out/eigenface-%-largest-4.pgm out/eigenface-%-largest-5.pgm out/eigenface-%-largest-6.pgm out/eigenface-%-largest-7.pgm out/eigenface-%-largest-8.pgm out/eigenface-%-largest-9.pgm out/eigenface-%-largest-10.pgm out/eigenface-%-smallest-1.pgm out/eigenface-%-smallest-2.pgm out/eigenface-%-smallest-3.pgm out/eigenface-%-smallest-4.pgm out/eigenface-%-smallest-5.pgm out/eigenface-%-smallest-6.pgm out/eigenface-%-smallest-7.pgm out/eigenface-%-smallest-8.pgm out/eigenface-%-smallest-9.pgm out/eigenface-%-smallest-10.pgm: ExperimentA/experiment out/training-%.dat
	ExperimentA/experiment info out/training-$*.dat -eig out/eigenface-$*-

.SECONDEXPANSION:
out/cmc-%.dat: ExperimentA/experiment out/training-$$(word 1,$$(subst -, ,$$*)).dat
	ExperimentA/experiment\
	    test\
	    Images/fb_$(word 1,$(subst -, ,$*))\
	    out/training-$(word 1,$(subst -, ,$*)).dat\
	    -i $(word 2,$(subst -, ,$*))\
	    -cmc $@\
	    -img out/classification-$*-\

out/correct-incorrect-%.txt: ExperimentA/experiment out/training-%.dat
	ExperimentA/experiment\
	    test\
	    Images/fb_$*\
	    out/training-$*.dat\
	    -c -inc\
	    >> $@

out/intruder-%.dat: ExperimentA/experiment out/training-intruder-%.dat
	ExperimentA/experiment\
	    test\
	    Images/fb_$*\
	    out/training-intruder-$*.dat\
	    -i 95\
	    -int $@

.SECONDEXPANSION:
out/compare-%.pdf: ExperimentA/plot.plt\
                   out/cmc-H-$$(word 1,$$(subst -, ,$$*)).dat out/cmc-H-$$(word 2,$$(subst -, ,$$*)).dat out/cmc-H-$$(word 3,$$(subst -, ,$$*)).dat\
				   out/cmc-L-$$(word 1,$$(subst -, ,$$*)).dat out/cmc-L-$$(word 2,$$(subst -, ,$$*)).dat out/cmc-L-$$(word 3,$$(subst -, ,$$*)).dat
	gnuplot -e "outfile='$@'"\
	        -e "infile1='out/cmc-H-$(word 1,$(subst -, ,$*)).dat'"\
	        -e "infile2='out/cmc-H-$(word 2,$(subst -, ,$*)).dat'"\
	        -e "infile3='out/cmc-H-$(word 3,$(subst -, ,$*)).dat'"\
	        -e "infile4='out/cmc-L-$(word 1,$(subst -, ,$*)).dat'"\
	        -e "infile5='out/cmc-L-$(word 2,$(subst -, ,$*)).dat'"\
	        -e "infile6='out/cmc-L-$(word 3,$(subst -, ,$*)).dat'"\
			-e "title1='High Resolution Data Set'"\
			-e "title2='Low Resolution Data Set'"\
	        -e "key1='$(word 1,$(subst -, ,$*))%'"\
	        -e "key2='$(word 2,$(subst -, ,$*))%'"\
	        -e "key3='$(word 3,$(subst -, ,$*))%'"\
	        ExperimentA/plot.plt

out/intruders.pdf: ExperimentA/intruder.plt out/intruder-H.dat out/intruder-L.dat
	gnuplot -e "outfile='$@'"\
	        -e "infile1='out/intruder-H.dat'"\
	        -e "infile2='out/intruder-L.dat'"\
	        -e "title1='H'"\
	        -e "title2='L'"\
	        ExperimentA/intruder.plt

# Figures needed for the report
report: out/mean-H.png out/compare-80-90-95.pdf out/intruders.pdf
report: out/correct-incorrect-H.txt out/correct-incorrect-L.txt
# Correctly and incorrectly classified images, respectively
report: Images/fa_H/00261_940128_fa.png Images/fa_H/00863_940307_fa.png Images/fa_H/01001_960627_fa.png Images/fa_L/00261_940128_fa.png Images/fa_L/00863_940307_fa.png Images/fa_L/01001_960627_fa.png
report: Images/fb_H/01001_960627_fb.png Images/fb_H/00261_940128_fb.png Images/fb_H/00863_940307_fb.png Images/fb_L/01001_960627_fb.png Images/fb_L/00261_940128_fb.png Images/fb_L/00863_940307_fb.png
report: Images/fa_H/00557_940519_fa.png Images/fa_H/00266_940128_fa.png Images/fa_H/01005_960627_fa.png Images/fa_L/00494_940519_fa.png Images/fa_L/00266_940128_fa.png Images/fa_L/00968_960627_fa.png
report: Images/fb_H/00556_940519_fb.png Images/fb_H/00212_940128_fb_a.png Images/fb_H/00695_941121_fb.png Images/fb_L/00770_960530_fb_a.png Images/fb_L/00212_940128_fb_a.png Images/fb_L/00695_941121_fb.png
report: out/eigenface-H-largest-1.png out/eigenface-H-largest-2.png out/eigenface-H-largest-3.png out/eigenface-H-largest-4.png out/eigenface-H-largest-5.png out/eigenface-H-largest-6.png out/eigenface-H-largest-7.png out/eigenface-H-largest-8.png out/eigenface-H-largest-9.png out/eigenface-H-largest-10.png
report: out/eigenface-H-smallest-1.png out/eigenface-H-smallest-2.png out/eigenface-H-smallest-3.png out/eigenface-H-smallest-4.png out/eigenface-H-smallest-5.png out/eigenface-H-smallest-6.png out/eigenface-H-smallest-7.png out/eigenface-H-smallest-8.png out/eigenface-H-smallest-9.png out/eigenface-H-smallest-10.png
report: out/eigenface-L-largest-1.png out/eigenface-L-largest-2.png out/eigenface-L-largest-3.png out/eigenface-L-largest-4.png out/eigenface-L-largest-5.png out/eigenface-L-largest-6.png out/eigenface-L-largest-7.png out/eigenface-L-largest-8.png out/eigenface-L-largest-9.png out/eigenface-L-largest-10.png
report: out/eigenface-L-smallest-1.png out/eigenface-L-smallest-2.png out/eigenface-L-smallest-3.png out/eigenface-L-smallest-4.png out/eigenface-L-smallest-5.png out/eigenface-L-smallest-6.png out/eigenface-L-smallest-7.png out/eigenface-L-smallest-8.png out/eigenface-L-smallest-9.png out/eigenface-L-smallest-10.png

Report/report.pdf: Report/report.tex report
	latexmk -pdf -cd -use-make -silent -pdflatex='pdflatex -interaction=batchmode -synctex=1' $<

clean:
	rm -rf $(OBJDIR)
	rm -f $(EXEC)
	rm -rf out
	find Images/ -type f -name '*.png' -delete
	cd Report/; latexmk -c

# Generate .png images from .pgm images. Needed for report, since pdfLaTeX doesn't support .pgm images
%.png: %.pgm
	pnmtopng $< > $@

%.png: %.ppm
	pnmtopng $< > $@

# Auto-Build .cpp files into .o
$(OBJDIR)/%.o: %.cpp
$(OBJDIR)/%.o: %.cpp $(DEPDIR)/%.d | $(DEPDIRS) $(OBJDIRS)
	$(CXX) $(DEPFLAGS) $(INCLUDES) $(CXXFLAGS) -c $< -o $@

# Make generated directories
$(DEPDIRS) $(OBJDIRS) out: ; @mkdir -p $@
$(DEPFILES):
include $(wildcard $(DEPFILES))