# Upgraded to c++17 to support the Filesystem library
# -lstdc++fs for gcc versions < 9.1
CXXFLAGS     = -std=c++17 -lstdc++fs -g -fopenmp -O3 -D_GLIBCXX_PARALLEL
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
REQUIRED_1   = out/mean.png
REQUIRED_2   = 
REQUIRED_OUT = $(REQUIRED_1) $(REQUIRED_2)

SOURCEDIRS   = $(call uniq, $(dir $(SOURCES)))
OBJDIRS      = $(addprefix $(OBJDIR)/, $(SOURCEDIRS))
DEPDIRS      = $(addprefix $(DEPDIR)/, $(SOURCEDIRS))
DEPFILES     = $(SOURCES:%.cpp=$(DEPDIR)/%.d)

.PHONY: all exec clean report out/A-table.tex out/B-table.tex
.SECONDARY:

# By default, make all executable targets and the outputs required for the homework
all: exec $(REQUIRED_OUT) Report/report.pdf
exec: $(EXEC)

# Executable Targets
ExperimentA/experiment: $(OBJDIR)/ExperimentA/main.o $(OBJDIR)/Common/image.o
	$(CXX) $(CXXFLAGS) $^ -o $@

### Experiment Outputs ###
out/mean.pgm: ExperimentA/experiment | out
	ExperimentA/experiment Images/fa_H -m out/mean.pgm

# Figures needed for the report
report: 

Report/report.pdf: Report/report.tex report
	latexmk -pdf -cd -use-make -silent -pdflatex='pdflatex -interaction=batchmode -synctex=1' $<

clean:
	rm -rf $(OBJDIR)
	rm -f $(EXEC)
	rm -rf out
	rm -f Images/*.png
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