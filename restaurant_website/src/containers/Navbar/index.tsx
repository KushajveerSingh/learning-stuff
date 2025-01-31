import styles from './Navbar.module.css';
import { useState } from 'react';
import Link from 'next/link';
import { GiHamburgerMenu } from 'react-icons/gi';
import { MdOutlineRestaurantMenu } from 'react-icons/md';

const Navbar = () => {
  const [toggleMenu, setToggleMenu] = useState(false);

  return (
    <nav className={styles.app__navbar}>
      <div className={styles.app__navbar_logo}>
        <img src="/gericht.png" alt="app logo" />
      </div>

      <ul className={styles.app__navbar_links}>
        <li className="p__opensans">
          <Link href="#home">
            <a href="#home">Home</a>
          </Link>
        </li>
        <li className="p__opensans">
          <Link href="#about">
            <a href="#about">About</a>
          </Link>
        </li>
        <li className="p__opensans">
          <Link href="#menu">
            <a href="#menu">Menu</a>
          </Link>
        </li>
        <li className="p__opensans">
          <Link href="#awards">
            <a href="#awards">Awards</a>
          </Link>
        </li>
        <li className="p__opensans">
          <Link href="#contact">
            <a href="#contact">Contact</a>
          </Link>
        </li>
      </ul>

      <div className={styles.app__navbar_login}>
        <Link href="#login">
          <a href="#login" className="p__opensans">
            Log In / Register
          </a>
        </Link>
        <div />
        <Link href="/">
          <a className="p__opensans">Book Table</a>
        </Link>
      </div>

      <div className={styles.app__navbar_smallscreen}>
        <GiHamburgerMenu color="#fff" fontSize={27} onClick={() => setToggleMenu(true)} />

        {toggleMenu && (
          <div
            className={`${styles.app__navbar_smallscreen_overlay} flex__center slide-bottom`}
          >
            <MdOutlineRestaurantMenu
              fontSize={27}
              className={styles.overlay__close}
              onClick={() => setToggleMenu(false)}
            />

            <ul className={styles.app__navbar_smallscreen_links}>
              <li className="p__opensans">
                <Link href="#home">
                  <a href="#home">Home</a>
                </Link>
              </li>
              <li className="p__opensans">
                <Link href="#about">
                  <a href="#about">About</a>
                </Link>
              </li>
              <li className="p__opensans">
                <Link href="#menu">
                  <a href="#menu">Menu</a>
                </Link>
              </li>
              <li className="p__opensans">
                <Link href="#awards">
                  <a href="#awards">Awards</a>
                </Link>
              </li>
              <li className="p__opensans">
                <Link href="#contact">
                  <a href="#contact">Contact</a>
                </Link>
              </li>
            </ul>
          </div>
        )}
      </div>
    </nav>
  );
};

export default Navbar;
