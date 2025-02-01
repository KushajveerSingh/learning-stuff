function validation() {
  var cvv = document.editForm.card_cvv;
  var name = document.editForm.card_name;
  var year = document.editForm.card_year;
  var month = document.editForm.card_month;
  var number = document.editForm.card_num;
  var check = document.editForm.something;
  var edit = document.editForm.card_option;
  if (checkAll(cvv, name, year, month, number, edit)) {
    if(lengthValidation(number, cvv, 16, 3, name, year, month, edit)){ 

    }
    else {
        check.value = "not_working";
        return false;     
    }
  } else {
    check.value = "not_working";
    return false;
  }
}

function checkAll(cvv, name, year, month, number, edit) {
  var cvv_len = cvv.value.length;
  var year_len = year.value.length;

  if (edit.value != "Delete") {
    if ((name.value.length == 0 || month.value == "Select" || number.value.length == 0 || year.value == "Select" || edit.value == "Select" || cvv_len == 0) && (name.value.length != 0 || month.value != "Select" || number.value.length != 0 || year.value != "Select" || edit.value != "Select" || cvv_len != 0)) {
        name.classList.add("border-danger");
        month.classList.add("border-danger");
        year.classList.add("border-danger");
        cvv.classList.add("border-danger");
        number.classList.add("border-danger");
        edit.classList.add("border-danger");
        alert("Fill out all fields");
    }
    if ((name.value.length != 0 && month.value != "Select" && number.value.length != 0 && year.value != "Select" && edit.value != "Select" && cvv_len != 0) || (name.value.length == 0 && month.value == "Select" && number.value.length == 0 && year.value == "Select" && edit.value == "Select" && cvv_len == 0)) {
        name.classList.remove("border-danger");
        month.classList.remove("border-danger");
        year.classList.remove("border-danger");
        cvv.classList.remove("border-danger");
        number.classList.remove("border-danger");
        edit.classList.remove("border-danger");
        return true;
    }
}
else {
    return true;
}

}

function lengthValidation(number, cvv, nLen, cLen, name, year, month, edit) {
  var number_len = number.value.length;
  var cvv_len = cvv.value.length;
  if ((name.value.length != 0 && month.value != "Select" && number.value.length != 0 && year.value != "Select" && edit.value != "Select" && cvv_len != 0)) {
    if (number_len != nLen || cvv_len != cLen) {
      number.classList.add("border-danger");
      cvv.classList.add("border-danger");
      alert("Incorrect length of Card number(16) or CVV(3)");
      return false;
    } else {
      number.classList.remove("border-danger");
      cvv.classList.remove("border-danger");
    }
  }
  return true;
}
