#!/usr/bin/python2
import sys
import arc
import time
import re

#get cmdline args
if not len(sys.argv)==3:
  print("Number of arguments must be 3 was: " + str(len(sys.argv)))
  sys.exit(-1)
controldir=sys.argv[1]
jobid=sys.argv[2]
filename=controldir+'/job.'+jobid+'.description'
locfile=controldir+'/job.'+jobid+'.3gdata'
inpfile=controldir+'/job.'+jobid+'.input'

print("starting 3GDataPlugin\n"+filename+"\n"+locfile+"\n\n")
#read in file
f=open(filename,'r')
rawdesc=f.read()
f.close()

jobdesc=arc.JobDescription()
jobdesclist=arc.JobDescriptionList()
if not jobdesc.Parse(rawdesc,jobdesclist):
   sys.exit(-1)

#extract staging info
dselements=[]
while jobdesclist.front().DataStaging.InputFiles.size()>0:
  dselements.append(jobdesclist.front().DataStaging.InputFiles.pop())

inputfiles=""
locelements=[]
#filter staging
for dsel in dselements:
  #check for attic or http
  proto=dsel.Sources[0].Protocol()
  url=dsel.Sources[0].str()
  matchlist = re.findall(".*?;md5=.*?:size=.*?$",url) #check whether url has md5 and size set
  if (proto=='attic' or proto=='http') and len(matchlist)==1 and matchlist[0]==url : #we can put http here also, but we'll need to check for md5 and size
    locelements.append(dsel)
  else:
    jobdesclist.front().DataStaging.InputFiles.append(dsel)
    if not dsel.Sources[0].Protocol()=='file':
      inputfiles = inputfiles +  dsel.Name + " " + dsel.Sources[0].str() + "\n"
    else:
      inputfiles = inputfiles + dsel.Name + "\n"

 
#write modified input
f=open(inpfile,'w')
f.write(inputfiles)
f.close()

print(inputfiles)
#print(jobdesc.UnParse()+"\n\n")
print("attic type files\n")
#append attic and http info to .3gdata
f=open(locfile,'a')
outstr=""
for elem in locelements:
  outstr=outstr + elem.Name+ " " + elem.Sources[0].str() + "\n"

print(outstr)
f.write(outstr)

f.close()

sys.exit(0)
