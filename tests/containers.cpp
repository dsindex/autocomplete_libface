#include <sparsetable.hpp>
#include <segtree.hpp>
#include <benderrmq.hpp>
#include <phrase_map.hpp>
#include <suggest.hpp>
#include <soundex.hpp>
#include <editdistance.hpp>

int
main() {
    segtree::test();
    sparsetable::test();
    benderrmq::test();

    phrase_map::test();
    _soundex::test();
    editdistance::test();

    return 0;
}
