rm -Rf modules libraries
drush make --yes --working-copy --no-core --contrib-destination=. feeds_test-dev.make
# Tests don't run with libraries ATM. Copy simplepie.inc into feeds libraries
# dir. See http://drupal.org/node/832962.
cp libraries/simplepie/simplepie.inc modules/feeds/libraries/
