# Maintainer: Sébastien Luttringer

pkgname=perl-test-output
pkgver=1.034
pkgrel=2
pkgdesc='Utilities to test STDOUT and STDERR messages'
arch=('any')
license=('GPL' 'PerlArtistic')
depends=('perl-sub-exporter'
  'perl-test-simple'
  'perl-test-pod'
  'perl-test-pod-coverage'
  'perl-capture-tiny')
url='https://search.cpan.org/dist/Test-Output'
options=('!emptydirs')
source=("https://search.cpan.org/CPAN/authors/id/B/BD/BDFOY/Test-Output-$pkgver.tar.gz")
sha512sums=('f7e6a121dbcaa6644b5128ba27c70894775bcb77e5c4abb3a2546d935cb1a99b3190230b2be76161377d0151665ec97303c48746b224d1b2301803e6082b0283')

build() {
  cd Test-Output-$pkgver
  USE_DEFAULT=1 perl Makefile.PL INSTALLDIRS=vendor
  make
}

check() {
  cd Test-Output-$pkgver
  make test
}

package() {
  cd Test-Output-$pkgver
  make install DESTDIR="$pkgdir/"
}

# vim:set ts=2 sw=2 et:
