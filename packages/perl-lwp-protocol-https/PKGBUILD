# Maintainer: Felix Yan <felixonmars@gmail.com>

pkgname=perl-lwp-protocol-https
pkgver=6.11
pkgrel=1
pkgdesc="Provide https support for LWP::UserAgent"
arch=('any')
url="https://metacpan.org/release/LWP-Protocol-https"
license=('PerlArtistic' 'GPL')
depends=('ca-certificates' 'perl-io-socket-ssl' 'perl-net-http' 'perl-libwww')
checkdepends=('perl-test-requiresinternet' 'perl-test-needs')
options=('!emptydirs')
source=("https://search.cpan.org/CPAN/authors/id/O/OA/OALDERS/LWP-Protocol-https-$pkgver.tar.gz")
sha512sums=('1e74c45898778c58d00eefbdd04a3ed47cf38164296278bec66c9b85f48ad635931873d2bf5423be8562df2c22b4bbf8eff0502bd29ba1c790bbbf76f3616ecf')

build() {
  cd LWP-Protocol-https-$pkgver
  perl Makefile.PL INSTALLDIRS=vendor
  make
}

check() {
  cd LWP-Protocol-https-$pkgver
  make test
}

package() {
  cd LWP-Protocol-https-$pkgver
  make DESTDIR="$pkgdir" install
}
