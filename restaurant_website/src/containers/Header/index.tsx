import styles from './Header.module.css';
import { SubHeading } from '../../components';

const Header = () => {
  return (
    <div className={`${styles.app__header} app__wrapper section__padding`} id="home">
      <div className="app__wrapper_info">
        <SubHeading title="Chase the new flavour" />
        <h1 className={styles.app__header_h1}>The Key to Fine Dining</h1>
        <p className="p__opensans" style={{ margin: '2rem 0' }}>
          Our creative, food and beverage program combines satisfying staples with
          imaginative twists. From boozy brunches and happy hours to family dinners, we
          have something for everyone.
        </p>
        <button type="button" className="custom__button">
          Explore Menu
        </button>
      </div>

      <div className="app__wrapper_img">
        <img src="/welcome.png" alt="header img" />
      </div>
    </div>
  );
};

export default Header;
