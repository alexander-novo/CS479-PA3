# Upgraded to c++17 to support the Filesystem library
CXXFLAGS     = -std=c++17 -fopenmp -O3 -D_GLIBCXX_PARALLEL -g
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
REQUIRED_1   = out/mean-H.pgm out/eigenface-H-largest-1.pgm
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

out/mean-%.pgm: ExperimentA/experiment out/training-%.dat
	ExperimentA/experiment info out/training-$*.dat -m $@

# Will also generate smallest-[1-10] and largest-[1-10]
out/eigenface-%-largest-1.pgm: ExperimentA/experiment out/training-%.dat
	ExperimentA/experiment info out/training-$*.dat -eig out/eigenface-$*-

.SECONDEXPANSION:
out/cmc-%.dat: ExperimentA/experiment out/training-$$(word 1,$$(subst -, ,$$*)).dat
	ExperimentA/experiment test Images/fb_$(word 1,$(subst -, ,$*)) out/training-$(word 1,$(subst -, ,$*)).dat -i $(word 2,$(subst -, ,$*)) -cmc $@ -img out/classification-$*-

.SECONDEXPANSION:
out/compare-%.pdf: ExperimentA/plot.plt out/cmc-$$(word 1,$$(subst -, ,$$*))-$$(word 2,$$(subst -, ,$$*)).dat out/cmc-$$(word 1,$$(subst -, ,$$*))-$$(word 3,$$(subst -, ,$$*)).dat out/cmc-$$(word 1,$$(subst -, ,$$*))-$$(word 4,$$(subst -, ,$$*)).dat
	gnuplot -e "outfile='$@'"\
	        -e "infile1='out/cmc-$(word 1,$(subst -, ,$*))-$(word 2,$(subst -, ,$*)).dat'"\
	        -e "infile2='out/cmc-$(word 1,$(subst -, ,$*))-$(word 3,$(subst -, ,$*)).dat'"\
	        -e "infile3='out/cmc-$(word 1,$(subst -, ,$*))-$(word 4,$(subst -, ,$*)).dat'"\
			-e "title1='$(word 2,$(subst -, ,$*))%'"\
			-e "title2='$(word 3,$(subst -, ,$*))%'"\
			-e "title3='$(word 4,$(subst -, ,$*))%'"\
			-e "dataSet='$(word 1,$(subst -, ,$*))'"\
	        ExperimentA/plot.plt

# Figures needed for the report
report: out/mean-H.png out/compare-H-80-90-95.pdf out/mean-L.png out/compare-L-80-90-95.pdf

Report/report.pdf: Report/report.tex report
	latexmk -pdf -cd -use-make -silent -pdflatex='pdflatex -interaction=batchmode -synctex=1' $<

clean:
	rm -rf $(OBJDIR)
	rm -f $(EXEC)
	rm -rf out
	rm -f Images/**.png
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