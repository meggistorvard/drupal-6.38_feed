; $Id: feeds_test-dev.make,v 1.14.2.1 2010/09/16 20:22:33 alexb Exp $
; Check out latest HEAD of Feeds.
; Use with patch for includes http://drupal.org/node/820992

api = 2

includes[drupal-org] = drupal-org.make

; Work with latest CVS version of Feeds
projects[feeds][type] = "module"
projects[feeds][download][type] = "cvs"
projects[feeds][download][module] = "contributions/modules/feeds"
projects[feeds][download][revision] = "DRUPAL-6--1"

; Work with latest CVS version of Job Scheduler
projects[job_scheduler][type] = "module"
projects[job_scheduler][download][type] = "cvs"
projects[job_scheduler][download][module] = "contributions/modules/job_scheduler"
projects[job_scheduler][download][revision] = "DRUPAL-6--1"

; Fix Call to undefined function drupal_realpath()
projects[simpletest][version] = 2.9
projects[simpletest][patch][] = "http://drupal.org/files/issues/587304-realpath-D6.patch"

; CTools fixes.
projects[ctools][patch][] = "http://drupal.org/files/issues/911362-2_static.patch"
projects[ctools][patch][] = "http://drupal.org/files/issues/911396-1_fix_notice.patch"

; Get Simplepie library.
libraries[simplepie][download][type] = "get"
libraries[simplepie][download][url] = "http://cloud.github.com/downloads/lxbarth/simplepie/SimplePie-1.2-0.tar.gz"
libraries[simplepie][directory_name] = "simplepie"
