# Maintainer: Felix Yan <felixonmars@archlinux.org>
# Contributor: Moritz Bunkus <moritz@bunkus.org>

pkgname=perl-cpan-meta-check
pkgver=0.018
pkgrel=2
pkgdesc="Verify requirements in a CPAN::Meta object"
arch=('any')
license=('PerlArtistic' 'GPL')
url='https://search.cpan.org/dist/CPAN-Meta-Check'
depends=('perl')
options=('!emptydirs')
source=("https://cpan.metacpan.org/authors/id/L/LE/LEONT/CPAN-Meta-Check-$pkgver.tar.gz")
sha512sums=('ea340287e4f14a5ea00f7fe0decc424ff3ac1af9615cf41905e36be37b1b5f3401d3d44d4aef1d0f4a253f46f0170e1a1607b669ab975ba78d7c1497f0583169')

build() {
  cd CPAN-Meta-Check-$pkgver
  perl Makefile.PL INSTALLDIRS=vendor
  make
}

check() {
  cd CPAN-Meta-Check-$pkgver
  make test
}

package() {
  cd CPAN-Meta-Check-$pkgver
  make DESTDIR="$pkgdir" install
}

# vim:set ts=2 sw=2 et:
