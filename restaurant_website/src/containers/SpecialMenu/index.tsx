import styles from './SpecialMenu.module.css';
import data from '../../data';
import { SubHeading, MenuItem } from '../../components';

const SpecialMenu = () => {
  return (
    <div className={`${styles.app__specialMenu} flex__center section__padding`} id="menu">
      <div className={styles.app__specialMenu_title}>
        <SubHeading title="Menu that fits your palatte" />
        <h1 className="headtext__cormorant">Today&apos;s Special</h1>
      </div>

      <div className={styles.app__specialMenu_menu}>
        <div className={`${styles.app__specialMenu_menu_wine} flex__center`}>
          <p className={styles.app__specialMenu_menu_heading}>Wine & Beer</p>
          <div className={styles.app__specialMenu_menu_items}>
            {data.wines.map((wine, index) => (
              <MenuItem
                key={wine.title + index}
                title={wine.title}
                price={wine.price}
                tags={wine.tags}
              />
            ))}
          </div>
        </div>

        <div className={styles.app__specialMenu_menu_img}>
          <img src="/menu.png" alt="menu__img" />
        </div>

        <div className={`${styles.app__specialMenu_menu_cocktails} flex__center`}>
          <p className={styles.app__specialMenu_menu_heading}>Cocktails</p>
          <div className={styles.app__specialMenu_menu_items}>
            {data.cocktails.map((cocktail, index) => (
              <MenuItem
                key={cocktail.title + index}
                title={cocktail.title}
                price={cocktail.price}
                tags={cocktail.tags}
              />
            ))}
          </div>
        </div>
      </div>

      <div style={{ marginTop: 15 }}>
        <button type="button" className="custom__button">
          View More
        </button>
      </div>
    </div>
  );
};

export default SpecialMenu;
